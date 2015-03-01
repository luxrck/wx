#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/device.h>

#define PIPE_MAXSIZE	(NDIRECT * BLKSIZE)

static void __blkread(struct file *file, char **buf, size_t count)
{
	int bno = bmap(file->inode, file->pos/BLKSIZE);
	struct superblock *sb = file->inode->sb;
	struct bcache *bc = bread(sb->dev, ZBLOCK(sb, bno));
	memcpy(*buf, bc->data + file->pos%BLKSIZE, count);
	brelse(bc);
	*buf += count;
	file->pos += count;
	if (file->pos >= PIPE_MAXSIZE)
		file->pos = 0;
}
static int pipe_read(struct file *file, char *buf, size_t count, off_t off)
{
	struct minode *mi = file->inode;
	printf("pipe_read: %d %d %d %d\n",mi->size, mi->wpos, mi->rpos, count);
	if (!mi->size)
		sleep(&mi->rpos);
	if (count > mi->size)
		count = mi->size;
	file->pos = mi->rpos;
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
		__blkread(file, &buf, count);
	mi->size -= count;
	mi->rpos = file->pos;
	wakeup(&mi->wpos);
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
	if (file->pos >= PIPE_MAXSIZE)
		file->pos = 0;
}
static int pipe_write(struct file *file, const char *buf, size_t count, off_t off)
{
	struct minode *mi = file->inode;
	if (mi->size >= PIPE_MAXSIZE)
		sleep(&mi->wpos);
	if (count > PIPE_MAXSIZE - mi->size)
		count = PIPE_MAXSIZE - mi->size;
	file->pos = mi->wpos;
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
	if (uc)
		__blkwrite(file, &buf, count);
	mi->size += count;
	mi->wpos = file->pos;
	wakeup(&mi->rpos);
	return count;
}

const static struct fileops pipe_fops = {
	.read = pipe_read,
	.write = pipe_write
};

static struct dev pipe_dev;

void pipeinit(void)
{
	pipe_dev.dev = MKDEV(DEV_PIPE, 0);
	pipe_dev.fops = &pipe_fops;
	pipe_dev.request = NULL;

	devregister(&pipe_dev);
}

static int __pipealloc(struct minode *mi, int rw)
{
	struct file *f = filealloc();
	int fd = fdalloc();

	if (!f) return -ENFILE;
	if (fd == -1) return -EMFILE;

	f->flags = rw ? O_WRONLY : O_RDONLY;
	f->mode = FT_PIPE;
	f->inode = mi;
	f->pos = 0;
	f->ref = 1;
	ctask->filp[fd] = f;

	return fd;
}
int pipe(int pfd[2])
{
	int fd = -1;
	
	struct superblock *sb = sbread(rootdev);
	struct minode *mi = ialloc(sb, IT_DEVICE);
	if (!mi) return -EMFILE;
	
	mi->dev = MKDEV(DEV_PIPE, 0);
	
	if ((fd = __pipealloc(mi, 0)) < 0) {
		iput(mi);
		return fd;
	}
	pfd[0] = fd;
	if ((fd = __pipealloc(mi, 1)) < 0) {
		close(pfd[0]);
		return fd;
	}
	pfd[1] = fd;
	return 0;
}