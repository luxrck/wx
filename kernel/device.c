#include <kernel/device.h>

struct dev *devices[NDEV];

void bllr(struct bcache *bc)
{
	dev_t dev = major(bc->dev);
	DEV(dev)->request(0, bc->data, dev, bc->blkno*2, 2);
}

void bllw(struct bcache *bc)
{
	dev_t dev = major(bc->dev);
	DEV(dev)->request(1, bc->data, dev, bc->blkno*2, 2);
}

void devinit(void)
{
	ideinit();
	consinit();
	pipeinit();
}