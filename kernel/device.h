#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <kernel/fs.h>

#define NDEV		10

//							dmajor	|	dminor
//							5-bits	|	3-bits
#define DEV_NONE	0	//	0		|
#define DEV_MEM		1	//	1		|
#define DEV_HD		2	//	2		|
#define DEV_TTYC	3	//	3		|
#define DEV_PIPE	4	//	4		|

struct dev {
	uint8_t dev;
	const struct fileops *fops;
	void (*request)(int rw, char *buf, dev_t dev, uint32_t secno, uint32_t count);
};

#define DEV(x)			(devices[x])
#define MKDEV(ma, mi)	(makedev(ma, mi))

#define rootdev			(MKDEV(DEV_HD, 0))

extern struct dev *devices[NDEV];

static inline void devregister(struct dev *dev)
{
	devices[major(dev->dev)] = dev;
}

void devinit(void);

void ideinit(void);
void consinit(void);
void pipeinit(void);

void bllr(struct bcache *bc);
void bllw(struct bcache *bc);

#endif /* !__DEVICE_H__ */