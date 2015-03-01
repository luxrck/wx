#ifndef __FS_H__
#define __FS_H__

#include <stdio.h>

#include <kernel/kernel.h>

// The type of `baddr' field of dinode is uint16_t,
// which means the max size of hd we support is 2^16*512 == 64MB

#define NPOF		20
#define NOINODE		256
#define NOFILE		512
#define NDIRECT		10
#define NIDIRECT	(BLKSIZE/2)
#define SECTSIZE	512
#define BLKSIZE		(2 * SECTSIZE)

#define FS_ROOTI	1
#define FS_NINODE	64
#define FL_MAXSIZE	(BLKSIZE*NOINODE+BLKSIZE*BLKSIZE/2)
#define FL_MAXBLKS	(NDIRECT + NIDIRECT)
#define IN_MAXREF	255

#define SZDIRENT	16
#define DIRNAMELEN	14

#define IT_REGULA	0
#define IT_DIRENT	1
#define IT_DEVICE	2

#define FT_REG		1
#define FT_CHR		2
#define FT_BLK		3
#define FT_FIFO		4
#define FT_PIPE		5

#define B_VALID		1
#define B_BUSY		2
#define B_DIRTY		4

#define NBCACHE		512
#define NBHASH(x)	(x % NBCACHE)
#define IPB			(BLKSIZE/(sizeof(struct dinode)))
#define DPB			(BLKSIZE/(sizeof(struct dir)))
#define IBLOCK(s,x)	((s)->ibb+(x-1)/IPB)
#define ZBLOCK(s,x)	((s)->zbb + (x-1))
#define IMBLOCK(s,x)((s)->ibmb + (x-1)/BLKSIZE/8)
#define ZMBLOCK(s,x)((s)->zbmb + (x-1)/BLKSIZE/8)
#define IOFF(x)		(((x-1) % IPB) * sizeof(struct dinode))
#define NSB			8

// sizeof(struct dinode) == 32B
// max file size: 512 + 10 == 522KB
struct dinode {
	uint8_t mode;		// file type and rwx bits(of cource we don't have it.)
	uint8_t nlink;
	uint32_t size;
	uint32_t ctime;
	uint16_t zone[NDIRECT+1];
} __attribute__((packed));

struct minode {
	uint8_t mode;
	uint8_t nlink;
	uint32_t size;
	time_t ctime;
	uint16_t zone[NDIRECT+1];
	uint8_t ref;
	struct superblock *sb;
	dev_t dev;
	uint16_t inum;
	uint8_t dirty;
	off_t rpos;
	off_t wpos;
} __attribute__((packed));

struct file {
	uint8_t flags;
	uint8_t mode;
	struct minode *inode;
	struct fileops *fops;
	uint8_t ref;
	off_t pos;
};

// linux/fs.h
struct fileops {
	off_t (*lseek) (struct file *, off_t, int);
	int (*read) (struct file *, char *, size_t, off_t);
	int (*write) (struct file *, const char *, size_t, off_t);
	int (*open) (struct minode *, struct file *);
	int (*release) (struct minode *, struct file *);
};

// sizeof(struct dir) == 16B
struct dir {
	uint16_t inum;	// inode number
	char name[DIRNAMELEN];
};

struct bcache {
	uint16_t blkno;
	uint8_t dev;
	uint8_t flags;
	uint8_t dirty;
	char data[BLKSIZE];
	struct bcache *prev;
	struct bcache *next;
};

// block size: 1KB
// inode size: 32B
struct superblock {
	uint16_t nzones;
	uint16_t ninodes;
	uint16_t ibmb;		// inode bitmap base block
	uint16_t zbmb;		// zone bitmap base block
	uint16_t ibb;		// inode base block
	uint16_t zbb;		// zone base block
	uint16_t magic;		// 0xCAFE
	uint16_t rooti;		// root inode number

	dev_t dev;
};

void fsinit(void);

struct superblock* sbread(uint8_t dev);
struct bcache* getblk(uint8_t dev, uint32_t blkno);
struct bcache* bread(uint8_t dev, uint32_t blkno);
void bwrite(struct bcache *bc);
void brelse(struct bcache *bc);

void inodezoneforeach(struct minode *mi, 
	int (*cb)(struct minode *mi, uint16_t index, uint16_t pb, void *data),
	void *data);

struct file* filealloc(void);
int fdalloc(void);

struct minode* iget(struct superblock *sb, uint16_t inum);
void iput(struct minode *mi);
uint16_t bmap(struct minode *mi, uint16_t bno);
struct minode* namei(const char *path, int *error, bool iparent);
struct minode* ialloc(struct superblock *sb, uint8_t mode);

#endif /* !__FS_H__ */