// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Pawel Jarosz <paweljarosz3691@gmail.com>
 */

#include <linux/errno.h>
#include <log.h>
#include <part.h>
#include <linux/usb/composite.h>
#include <asm/arch-rockchip/f_rockusb.h>

int rockusb_mmc_erase(struct rockusb_dev_desc *desc, unsigned int start,
					   unsigned int blkcnt)
{
    return blk_derase((struct blk_desc *)desc->priv, start, blkcnt);
}

int rockusb_mmc_read(struct rockusb_dev_desc *desc, unsigned int lba,
					 unsigned int blkcount, char *buf)
{
    return blk_dread((struct blk_desc *)desc->priv, lba, blkcount, buf);
}

int rockusb_mmc_write(struct rockusb_dev_desc *desc, unsigned int lba,
					  unsigned int blkcount, char *buf)
{
    return blk_dwrite((struct blk_desc *)desc->priv, lba, blkcount, buf);
}

int rockusb_fill_mmc_dev(struct rockusb_dev_desc *desc, char *dev_type,
                         unsigned int dev_index)
{
    struct blk_desc *blk_desc;

	blk_desc = blk_get_dev(dev_type, dev_index);
	if (!blk_desc || blk_desc->type == DEV_TYPE_UNKNOWN)
		return -ENODEV;

    desc->blksz = blk_desc->blksz;
    desc->priv = (void *) blk_desc;
    desc->rockusb_erase = rockusb_mmc_erase;
    desc->rockusb_read = rockusb_mmc_read;
    desc->rockusb_write = rockusb_mmc_write;

    return 0;
}