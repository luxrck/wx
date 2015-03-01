#include <errno.h>
#include <string.h>

#include <kernel/mm.h>
#include <kernel/fs.h>
#include <kernel/kclock.h>
#include <kernel/device.h>
#include <kernel/proc.h>

static struct bcache bcache[NBCACHE], *bchead, *bctail;
static struct minode inodes[NOINODE];
static struct superblock sb[NSB];
static struct file files[NOFILE];

static const char* basename(const char *path)
{
	const char *name = path + strlen(path);
	while (name > path && *name != '/') name--;
	return *name == '/' ? name + 1 : name;
}

static bool bitmapget(char *map, uint16_t i)
{
	return map[(i-1)/8] & (1 << ((i-1) % 8));
}

static void bitmapset(char *map, uint32_t i, uint8_t val)
{
	if (val)
		map[(i-1)/8] |= (1 << ((i-1) % 8));
	else
		map[(i-1)/8] &= ~(1 << ((i-1) % 8));
}

struct superblock* sbread(uint8_t dev)
{
	int firstavasbi = -1;
	for (int i=0; i<NSB; i++) {
		if (sb[i].magic && sb[i].dev == dev)
			return &sb[i];
		if (firstavasbi == -1 && !sb[i].magic)
			firstavasbi = i;
	}
	if (firstavasbi == -1)
		return NULL;
	struct bcache *bc = bread(dev, 1);
	struct superblock *ssb = (struct superblock*)bc->data;
	sb[firstavasbi] = *ssb;
	sb[firstavasbi].dev = dev;
	brelse(bc);
	return &sb[firstavasbi];
}

void bcinit(void)
{
	bchead = &bcache[0];
	bctail = &bcache[NBCACHE-1];
	for (int i=1; i<NBCACHE; i++) {
		bcache[(i-1)%NBCACHE].next = &bcache[i];
		bcache[i].prev = &bcache[(i-1)%NBCACHE];
	}
}

static void bfree(uint8_t dev, uint16_t bno)
{
	struct superblock *sb = sbread(dev);
	struct bcache *bc = bread(sb->dev, ZMBLOCK(sb, bno));
	bitmapset(bc->data, bno % BLKSIZE, 0);
	bwrite(bc);
	brelse(bc);
	bc = bread(sb->dev, ZBLOCK(sb, bno));
	memset(bc->data, 0, BLKSIZE);
	bwrite(bc);
	brelse(bc);
}
static uint16_t __balloc(struct superblock *sb, uint16_t zbmno, uint16_t bms)
{
	struct bcache *bc = bread(sb->dev, zbmno);
	for (int i=1; i<=bms; i++) {
		if (bitmapget(bc->data, i))
			continue;
		bitmapset(bc->data, i, 1);
		bwrite(bc);
		brelse(bc);
		bc = bread(sb->dev, ZBLOCK(sb, i));
		memset(bc->data, 0, BLKSIZE);
		bwrite(bc);
		brelse(bc);
		return i;
	}
	brelse(bc);
	return 0;
}
uint16_t balloc(uint8_t dev)
{
	int r = 0;
	struct superblock *sb = sbread(dev);
	for (int i=sb->zbmb; i<sb->ibb; i++)
		if ((r = __balloc(sb, i, BLKSIZE * 8)))
			return r;
	return 0;
}

struct bcache* getblk(uint8_t dev, uint32_t blkno)
{
	if (ctask)
		procsavecontext(&ctask->ec);

	struct bcache *bc = bchead;
	while (bc) {
		if (bc->dev == dev && bc->blkno == blkno) {
			if (!(bc->flags & B_BUSY))
				goto ret;
			scheduler();
		}
		bc = bc->next;
	}

loop:
	bc = bctail;
	while (bc) {
		if (!(bc->flags & (B_BUSY | B_DIRTY))) {
			bc->dev = dev;
			bc->blkno = blkno;
			bc->flags = B_BUSY;
			goto ret;
		}
		bc = bc->prev;
	}
	bwrite(bctail);
	goto loop;

ret:
	if (ctask)
		ctask->ec.eip = 0;
	return bc;
}

struct bcache* bread(uint8_t dev, uint32_t blkno)
{
	struct bcache *bc = getblk(dev, blkno);
	if (!(bc->flags & B_VALID))
		bllr(bc);
	bc->flags = B_VALID | B_BUSY;
	return bc;
}

void bwrite(struct bcache *bc)
{
	bllw(bc);
	bc->flags = B_VALID;
}

void brelse(struct bcache *bc)
{
	bc->flags &= ~B_BUSY;
	if (bc == bchead)
		return;
	if (bc->next)
		bc->next->prev = bc->prev;
	if (bc->prev)
		bc->prev->next = bc->next;
	bc->next = bchead;
	bc->prev = NULL;
	bchead->prev = bc;
	bchead = bc;
}

void inodezoneforeach(struct minode *mi, 
	int (*cb)(struct minode *mi, uint16_t i, uint16_t b, void *data),
	void *data)
{
	for (int i=0; i<NDIRECT; i++) {
		if (!mi->zone[i])
			continue;
		if (!cb(mi, i, ZBLOCK(mi->sb, mi->zone[i]), data))
			return;
	}

	if (!mi->zone[NDIRECT])
		return;
	struct bcache *bc = bread(mi->dev, ZBLOCK(mi->sb, mi->zone[NDIRECT]));
	uint16_t *blks = (uint16_t*)bc->data;
	for (int i=0; i<NIDIRECT; i++)
		if (blks[i] && !cb(mi, i+NDIRECT, ZBLOCK(mi->sb, blks[i]), data))
			break;
	brelse(bc);
}

struct minode* iget(struct superblock *sb, uint16_t inum)
{
	if (major(sb->dev) != DEV_HD || inum < 1 || inum > NOINODE)
		return NULL;
	struct minode *mi = NULL, *firstavainode = NULL;
	for (int i=0; i<NOINODE; i++) {
		if (inodes[i].sb == sb && inodes[i].inum == inum) {
			mi = &inodes[i];
			goto ret;
		}
		if (!firstavainode && !inodes[i].ref)
			firstavainode = &inodes[i];
	}
	if (!firstavainode)
		return NULL;

	mi = firstavainode;
	memset(mi, 0, sizeof(struct minode));
	mi->sb = sb;
	mi->dev = sb->dev;
	mi->inum = inum;
	mi->dirty = 0;

	struct bcache *bc = bread(sb->dev, IBLOCK(sb, inum));
	memcpy(mi, bc->data+IOFF(inum), sizeof(struct dinode));
	brelse(bc);
ret:
	mi->ref++;
	return mi;
}

static int __freeblock(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	bfree(mi->dev, b);
	return 1;
}
static int __putblock(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	struct bcache *bc = getblk(mi->dev, b);
	if (bc->flags & B_DIRTY)
		bwrite(bc);
	brelse(bc);
	return 1;
}
static void isync(struct minode *mi)
{
	struct superblock *sb = mi->sb;
	struct bcache *bc = bread(sb->dev, IBLOCK(sb, mi->inum));
	memcpy(bc->data+IOFF(mi->inum), mi, sizeof(struct dinode));
	bwrite(bc);
	brelse(bc);
}

static void ifree(struct minode *mi)
{
	inodezoneforeach(mi, __freeblock, NULL);
	struct bcache *bc = bread(mi->dev, IMBLOCK(mi->sb, mi->inum));
	bitmapset(bc->data, mi->inum % IPB, 0);
	bwrite(bc);
	brelse(bc);
	bc = bread(mi->sb->dev, IBLOCK(mi->sb, mi->inum));
	memset(bc+IOFF(mi->inum), 0, sizeof(struct dinode));
	bwrite(bc);
	brelse(bc);
}
void iput(struct minode *mi)
{
	if (!mi) return;
	if (!mi->ref || !--mi->ref) {
		if (!mi->nlink) {
			mi->mode = 0;
			ifree(mi);
		}
		if (*pteget(kpgdir, mi, 0) & PTE_D) {
			isync(mi);
			inodezoneforeach(mi, __putblock, NULL);
		}
	}
}

uint16_t bmap(struct minode *mi, uint16_t bno)
{
	if (bno >= NDIRECT + NIDIRECT)
		panic("file is too big.");
	
	if (bno < NDIRECT) {
		if (!mi->zone[bno])
			mi->zone[bno] = balloc(mi->sb->dev);
		return mi->zone[bno];
	}
	
	if (!mi->zone[NDIRECT]) {
		mi->zone[NDIRECT] = balloc(mi->sb->dev);
		return mi->zone[NDIRECT];
	}

	bno -= NDIRECT;
	struct bcache *bc = bread(mi->dev, ZBLOCK(mi->sb, mi->zone[NDIRECT]));
	uint16_t *bm = (uint16_t*)bc->data;
	if (!bm[bno])
		bm[bno] = balloc(mi->sb->dev);
	brelse(bc);
	return bm[bno];
}

static int __getdirent(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	struct dir *dirent = (struct dir*)data;
	struct bcache *bc = bread(mi->dev, b);
	struct dir *dirs = (struct dir*)bc->data;
	for (int i=0; i<DPB; i++) {
		if (dirs[i].inum)
		if (!strcmp(dirs[i].name, dirent->name)) {
			dirent->inum = dirs[i].inum;
			brelse(bc);
			return 0;
		}
	}
	brelse(bc);
	return 1;
}
void __parseurl(const char **path, char *store)
{
	while (**path  == '/') (*path)++;
	const char *npath = strstr(*path, "/");
	if (!npath)
		npath = *path + strlen(*path);
	strlcpy(store, *path, npath - *path + 1);
	*path = *npath ? npath + 1 : npath;
}
struct minode* __rooti(const char **path)
{
	struct minode *ci = NULL;
	if (**path == '/') {
		ci = iget(ctask->sb, ctask->root);
		(*path)++;
	} else
		ci = iget(ctask->sb, ctask->cwd);
	
	// check wd is still alive or not
	if (!ci->nlink) return NULL;

	//ci->ref++;
	return ci;
}
int __pathi(struct minode **ci, const char **path)
{
	int err = 0;
	struct dir dir = {0};
	struct minode *pi = *ci;

	__parseurl(path, dir.name);

	if (pi->mode != IT_DIRENT && **path) {
		err = -ENOTDIR;
		goto ret;
	}

	if (pi->inum == ctask->root && !strcmp(dir.name, ".."))
		return 0;

	inodezoneforeach(pi, __getdirent, &dir);

	if (!dir.inum) {
		err = -ENOENT;
		goto ret;
	}

	struct superblock *sb = pi->sb;
	*ci = iget(sb, dir.inum);

ret:
	iput(pi);
	return err;
}
struct minode* namei(const char *path, int *error, bool iparent)
{
	int vinodes[64] = {0}, vi = 0, err = 0;
	const char *name = basename(path);
	struct minode *ci = __rooti(&path);
	
	if (error) *error = 0;

	if (!ci) return NULL;

	vinodes[vi++] = ci->inum;
	
	while ((iparent && path < name) || (!iparent && *path)) {
		if ((err = __pathi(&ci, &path)))
			goto err;
		if (!*path)
			break;
		for (int i=0; i<vi; i++)
			if (vi >= 64 || vinodes[i] == ci->inum) {
				err = -ELOOP;
				goto err;
			}
		vinodes[vi++] = ci->inum;
	}

	return ci;

err:
	if (err && error) *error = err;
	return NULL;
}

struct minode* __ialloc(struct superblock *sb, uint16_t ibmno,
	uint16_t bms, uint8_t mode)
{
	struct bcache *bc = bread(sb->dev, ibmno);
	for (int i=1; i<=bms; i++) {
		if (bitmapget(bc->data, i))
			continue;
		bitmapset(bc->data, i, 1);
		bwrite(bc);
		brelse(bc);
		bc = bread(sb->dev, IBLOCK(sb, i));
		struct dinode *di = (struct dinode*)bc->data + i - 1;
		memset(di, 0, sizeof(struct dinode));
		di->mode = mode;
		di->nlink = 1;
		di->ctime = time(NULL);
		bwrite(bc);
		brelse(bc);
		return iget(sb, i);
	}
	return NULL;
}
struct minode* ialloc(struct superblock *sb, uint8_t mode)
{
	struct minode *r = NULL;
	for (int i=sb->ibmb; i<sb->zbmb; i++)
		if ((r = __ialloc(sb, i, BLKSIZE * 8, mode)))
			return r;
	return NULL;
}

static int __setdirent(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	struct dir *dirent = (struct dir*)data;
	struct bcache *bc = bread(mi->dev, b);
	struct dir *dirs = (struct dir*)bc->data;
	for (int i=0; i<DPB; i++) {
		if (!dirs[i].inum) {
			dirs[i].inum = dirent->inum;
			strcpy(dirs[i].name, dirent->name);
			bwrite(bc);
			brelse(bc);
			return 0;
		}
	}
	bmap(mi, i+1);
	mi->size += BLKSIZE;
	brelse(bc);
	isync(mi);
	return 1;
}
static struct minode* dirset(struct minode *pi, const char *name, int mode, int *err, int ino)
{
	int error = 0;
	struct minode *mi = NULL;
	if (!*name) goto ret;
	if (!ino) {
		mi = ialloc(pi->sb, mode);
		ino = mi->inum;
	}
	struct dir dirent;
	dirent.inum = ino;
	strcpy(dirent.name, name);
	pi->size = BLKSIZE;
	inodezoneforeach(pi, __setdirent, &dirent);

ret:
	if (error && err)
		*err = error;
	return mi;
}

struct file* filealloc(void)
{
	for (int i=0; i<NOFILE; i++) {
		if (!files[i].ref) {
			memset(&files[i], 0, sizeof(struct file));
			return &files[i];
		}
	}
	return NULL;
}

int fdalloc(void)
{
	for (int i=0; i<NPOF; i++)
		if (!ctask->filp[i])
			return i;
	return -1;
}

void fsinit(void)
{
	bcinit();
}

/*
 * High-Level functions
 */

int open(const char *path, int flags)
{
	if (!(flags & O_RDWR))
		flags |= O_RDONLY;

	int error = 0;
	struct minode *mi = namei(path, &error, 1);
	if (error) return error;
	iput(mi);
	mi = namei(path, &error, 0);
	uint8_t mm = 0;
	if (mi) {
		iput(mi);
		mm = mi->mode;
	}

	if ((flags & O_CREAT) && !error)
		return -EEXIST;
	if (!(flags & O_CREAT) && error)
		return error;
	if (mm == IT_DIRENT && flags & O_WRONLY)
		return -EISDIR;

	struct file *file = filealloc();
	int fd = fdalloc();
	
	if (!file) return -ENFILE;
	if (fd == -1) return -EMFILE;

	if (flags & O_CREAT) {
		struct minode *pi = namei(path, &error, 1);
		if (!(mi = dirset(pi, basename(path), IT_REGULA, &error, 0))) {
			iput(pi);
			return error;
		}
		iput(pi);
	} else {
		mi = namei(path, &error, 0);
	}

	int r = 0;
	struct dev *dev = DEV(major(mi->dev));
	if (mi->mode == IT_DEVICE && dev->fops && dev->fops->open)
		if ((r = dev->fops->open(mi, file) < 0))
		return r;

	file->flags = flags;
	if (!file->mode)
		file->mode = FT_REG;
	file->inode = mi;
	file->pos = 0;
	file->ref = 1;
	ctask->filp[fd] = file;

	return fd;
}

int close(int fd)
{
	struct file *file = ctask->filp[fd];
	if (!file) return -EBADF;
	if (--file->ref) goto ret;
	
	struct minode *mi = file->inode;
	uint8_t dev = mi->dev;

	if (mi->mode == IT_DEVICE)
		DEV(major(dev))->fops->release(file->inode, file);

	iput(mi);

	memset(file, 0, sizeof(struct file));

ret:
	ctask->filp[fd] = 0;
	return 0;
}

static void __blkread(struct file *file, char **buf, size_t count)
{
	int bno = bmap(file->inode, file->pos/BLKSIZE);
	struct superblock *sb = file->inode->sb;
	struct bcache *bc = bread(sb->dev, ZBLOCK(sb, bno));
	memcpy(*buf, bc->data + file->pos%BLKSIZE, count);
	brelse(bc);
	*buf += count;
	file->pos += count;
}
int read(int fd, char *buf, size_t count)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	if (!file || !(file->flags & O_RDONLY)) return -EBADF;

	if (!count)
		return 0;
	if (!pteget(ctask->pgdir, buf, 0))
		return -EFAULT;
	if (!file)
		return -EINVAL;
	if (file->inode->mode & IT_DIRENT)
		return -EISDIR;

	uint8_t dev = file->inode->dev;
	if (file->inode->mode == IT_DEVICE)
		return DEV(major(dev))->fops->read(file, buf, count, file->pos);

	if (file->pos >= file->inode->size)
		return 0;

	if (file->pos + count >= file->inode->size)
		count = file->inode->size - file->pos;

	size_t fc = BLKSIZE - file->pos % BLKSIZE;
	size_t uc = count;
	if (uc < fc)
		goto r3;
	__blkread(file, &buf, fc);
	uc -= fc;
	while (uc > BLKSIZE) {
		__blkread(file, &buf, BLKSIZE);
		uc -= BLKSIZE;
	}
r3:
	if (uc)
		__blkread(file, &buf, uc);

	return count;
}

static void __blkwrite(struct file *file, const char **buf, size_t count)
{
	int bno = bmap(file->inode, file->pos/BLKSIZE);
	struct superblock *sb = file->inode->sb;
	struct bcache *bc = bread(sb->dev, ZBLOCK(sb, bno));
	memcpy(bc->data + file->pos%BLKSIZE, *buf, count);
	bwrite(bc);
	brelse(bc);
	*buf += count;
	file->pos += count;
	if (file->pos > file->inode->size)
		file->inode->size = file->pos;
}
int write(int fd, const char *buf, size_t count)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	if (!file || !(file->flags & O_WRONLY)) return -EBADF;

	if (!count) return 0;

	if (file->pos >= FL_MAXSIZE)
		return -ENOSPC;
	if (!file)
		return -EINVAL;
	if (file->flags & O_APPEND)
		file->pos = file->inode->size;

	uint8_t dev = file->inode->dev;
	if (file->inode->mode == IT_DEVICE)
		return DEV(major(dev))->fops->write(file, buf, count, file->pos);

	if (file->pos + count > FL_MAXSIZE)
		count = FL_MAXSIZE - file->pos;

	size_t fc = BLKSIZE - file->pos % BLKSIZE;
	size_t uc = count;
	if (uc < fc)
		goto r3;
	__blkwrite(file, &buf, fc);
	uc -= fc;
	while (uc > BLKSIZE) {
		__blkwrite(file, &buf, BLKSIZE);
		uc -= BLKSIZE;
	}
r3:
	
	if (uc);
		__blkwrite(file, &buf, uc);
	
	isync(file->inode);
	return count;
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
	if (mode > 4 || mode < 1) return -EINVAL;

	int error = 0;
	struct minode *mi = namei(path, &error, 1);
	if (error) return error;
	iput(mi);
	mi = namei(path, &error, 0);
	if (mi) {
		iput(mi);
		return -EEXIST;
	}

	mode == S_IFREG ? IT_REGULA : IT_DEVICE;
	
	struct minode *pi = namei(path, &error, 1);
	if (!(mi = dirset(pi, basename(path), mode, &error, 0))) {
		iput(pi);
		return error;
	}
	iput(pi);

	mi->dev = dev;
	iput(mi);

	return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	if (!file || !(file->flags & O_RDONLY)) return -EBADF;
	if (file->mode == FT_PIPE
		|| file->mode == FT_FIFO) return -ESPIPE;

	switch (whence) {
	case SEEK_SET:
		file->pos = offset;
		break;
	case SEEK_CUR:
		file->pos += offset;
		break;
	case SEEK_END:
		file->pos = file->inode->size + offset;
		break;
	default:
		return -EINVAL;
	}

	return file->pos;
}

int dup(int ofd)
{
	struct file *file = ctask->filp[ofd];
	if (!file) return -EBADF;
	int nfd = fdalloc();
	if (nfd == -1) return -EMFILE;
	ctask->filp[nfd] = file;
	file->ref++;
	return nfd;
}

int dup2(int ofd, int nfd)
{
	struct file *file = ctask->filp[ofd];
	if (!file || nfd >= NPOF || nfd < 0) return -EBADF;
	if (ctask->filp[nfd]) close(nfd);
	ctask->filp[nfd] = file;
	file->ref++;
	return nfd;
}

int mkdir(const char *path)
{
	int error = 0;
	struct minode *mi = namei(path, &error, 1);
	if (error) return error;
	iput(mi);
	mi = namei(path, &error, 0);
	if (mi) {
		iput(mi);
		return -EEXIST;
	}
	
	struct minode *pi = namei(path, &error, 1);
	if (!(mi = dirset(pi, basename(path), IT_DIRENT, &error, 0))) {
		iput(pi);
		return error;
	}
	
	bmap(mi, 0);
	dirset(mi, ".", IT_DIRENT, &error, mi->inum);
	dirset(mi, "..", IT_DIRENT, &error, pi->inum);
	iput(pi);
	iput(mi);

	return 0;
}

static int __dirckempty(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	struct bcache *bc = bread(mi->dev, b);
	struct dir *dirs = (struct dir*)bc->data;
	for (int i=0; i<DPB; i++) {
		char *nm = dirs[i].name;
		if (*nm && strcmp(nm, ".") && strcmp(nm, "..")) {
			*(int*)data = 1;
			brelse(bc);
			return 0;
		}
	}
	brelse(bc);
	*(int*)data = 0;
	return 1;
}
static int __rmdirent(struct minode *mi, uint16_t i, uint16_t b, void *data)
{
	struct dir *dirent = (struct dir*)data;
	struct bcache *bc = bread(mi->dev, b);
	struct dir *dirs = (struct dir*)bc->data;
	for (int i=0; i<DPB; i++) {
		if (dirs[i].inum == dirent->inum
			&& !strcmp(dirs[i].name, dirent->name)) {
			memset(&dirs[i], 0, sizeof(struct dir));
			bwrite(bc);
			brelse(bc);
			return 0;
		}
	}
	brelse(bc);
	return 1;
}
int rmdir(const char *path)
{
	int error = 0;
	struct minode *mi = namei(path, &error, 1);
	if (error) return error;
	iput(mi);

	mi = namei(path, &error, 0);
	if (!mi) return -ENOENT;
	if (mi->mode != IT_DIRENT) {
		iput(mi);
		return -ENOTDIR;
	}

	const char *name = basename(path);
	if (!strcmp(name, ".")) return -EINVAL;
	if (!strcmp(name, "..")) return -ENOTEMPTY;
	inodezoneforeach(mi, __dirckempty, &error);
	if (error) {
		iput(mi);
		return -ENOTEMPTY;
	}
	struct dir dirent;
	dirent.inum = mi->inum;
	strcpy(dirent.name, name);
	struct minode *pi = namei(path, &error, 1);
	inodezoneforeach(pi, __rmdirent, &dirent);
	iput(pi);

	mi->nlink--;
	iput(mi);
	return 0;
}

int link(const char *op, const char *np)
{
	int error = 0;
	struct minode *mo = namei(op, &error, 0);
	if (error) return error;
	uint8_t mm = mo->mode;
	if (mm == IT_DIRENT) {
		iput(mo);
		return -EISDIR;
	}
	struct superblock *sb = mo->sb;
	struct dir dirent;
	dirent.inum = mo->inum;
	strcpy(dirent.name, basename(np));
	iput(mo);

	if (mm == IT_DIRENT) return -EPERM;
	if (mm == IN_MAXREF) return -EMLINK;

	struct minode *mn = namei(np, &error, 1);
	if (error) return error;
	struct minode *mt = namei(np, &error, 0);
	if (mt) {
		iput(mt);
		return -EEXIST;
	}

	inodezoneforeach(mn, __setdirent, &dirent);
	mo = iget(sb, dirent.inum);
	mo->nlink++;
	iput(mo);
	iput(mn);
	return 0;
}

int unlink(const char *path)
{
	int error = 0;
	struct minode *mi = namei(path, &error, 0);
	if (error) return error;
	if (mi->mode == IT_DIRENT) {
		iput(mi);
		return -EISDIR;
	}

	struct superblock *sb = mi->sb;
	const char *name = basename(path);

	if (!strcmp(name, ".") || !strcmp(name, "..")) {
		iput(mi);
		return -EINVAL;
	}
	
	struct dir dirent;
	dirent.inum = mi->inum;
	strcpy(dirent.name, name);

	mi = namei(path, &error, 1);
	inodezoneforeach(mi, __rmdirent, &dirent);

	mi->dirty = 1;
	iput(mi);

	mi = iget(sb, dirent.inum);
	mi->nlink--;
	iput(mi);
	
	return 0;
}

int rename(const char *op, const char *np)
{
	int error = 0;
	struct minode *mo = namei(op, &error, 0);
	if (error) return error;
	uint8_t mm = mo->mode;
	struct superblock *sb = mo->sb;
	struct dir dirent;
	dirent.inum = mo->inum;
	iput(mo);

	if (mm == IT_DIRENT) return -EPERM;

	struct minode *mn = namei(np, &error, 1);
	if (error) return error;
	struct minode *mt = namei(np, &error, 0);
	if (mt) {
		iput(mt);
		return -EEXIST;
	}
	mo = namei(op, &error, 1);
	strcpy(dirent.name, basename(op));
	inodezoneforeach(mo, __rmdirent, &dirent);
	strcpy(dirent.name, basename(np));
	inodezoneforeach(mn, __setdirent, &dirent);
	iput(mo);
	iput(mn);
	return 0;
}

int fstat(int fd, struct stat *st)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	if (!file) return -EBADF;

	struct minode *mi = file->inode;
	st->st_dev = mi->sb->dev;
	st->st_ino = mi->inum;
	st->st_mode = mi->mode;
	st->st_nlink = mi->nlink;
	st->st_rdev = mi->dev;
	st->st_size = mi->size;

	return 0;
}

int stat(const char *path, struct stat *st)
{
	int r = 0;
	if ((r = open(path, O_RDONLY)) < 0)
		return r;
	return fstat(r, st);
}

int fchdir(int fd)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	if (!file || file->inode->mode != IT_DIRENT) return -EBADF;

	ctask->cwd = file->inode->inum;
	return 0;
}

int chdir(const char *path)
{
	int f = 0;
	if ((f = open(path, O_RDONLY)) < 0)
		return f;
	int r = fchdir(f);
	close(f);
	return r;
}

static int __readdirent(struct file *f, uint16_t b, uint16_t off, struct dir *dirent)
{
	memset(dirent, 0, sizeof(struct dir));
	struct bcache *bc = bread(f->inode->dev, b);
	struct dir *dirs = (struct dir*)bc->data;
	for (int i=off; i<DPB; i++) {
		f->pos += SZDIRENT;
		if (dirs[i].inum) {
			dirent->inum = dirs[i].inum;
			strcpy(dirent->name, dirs[i].name);
			brelse(bc);
			return 0;
		}
	}
ret:
	brelse(bc);
	return 1;
}
int readdir(int fd, struct dir *sd)
{
	if (fd >= NPOF) return -EBADF;
	struct file *file = ctask->filp[fd];
	struct minode *mi = file->inode;
	if (!file || mi->mode != IT_DIRENT || !(file->flags & O_RDONLY)) return -EBADF;

	for (int i=file->pos/BLKSIZE; i<NDIRECT; i++) {
		if (!mi->zone[i]) {
			file->pos += BLKSIZE;
			continue;
		}
		if (!__readdirent(file, ZBLOCK(mi->sb, mi->zone[i]),
			(file->pos % BLKSIZE) / SZDIRENT, sd))
			return 1;
	}

	if (!mi->zone[NDIRECT])
		return 0;
	struct bcache *bc = bread(mi->dev, ZBLOCK(mi->sb, mi->zone[NDIRECT]));
	uint16_t *blks = (uint16_t*)bc->data;
	for (int i=0; i<NIDIRECT; i++) {
		if (!blks[i]) {
			file->pos += BLKSIZE;
			continue;
		}
		if (!__readdirent(file, ZBLOCK(mi->sb, blks[i]),
			(file->pos % BLKSIZE) / SZDIRENT, sd))
			return 1;
	}
	brelse(bc);
	return 0;
}

void sync(void)
{
	for (int i=0; i<NBCACHE; i++) {
		if (bcache[i].flags & B_DIRTY)
			bwrite(&bcache[i]);
	}
}