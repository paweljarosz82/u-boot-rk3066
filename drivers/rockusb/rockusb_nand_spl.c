// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Pawel Jarosz <paweljarosz3691@gmail.com>
 */
#define DEBUG

#include <common.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <linux/usb/composite.h>
#include <asm/arch-rockchip/f_rockusb.h>
#include <asm/arch-rockchip/rknand.h>
#include <linux/errno.h>
#include <linux/io.h>

#define ROCKCHIP_NAND_BLK_SZ	0x200
#define INIT_COMPLETE_MAGIC		0x12345678

int rockusb_nand_erase(struct rockusb_dev_desc *desc, unsigned int start,
		       unsigned int blkcnt)
{
	return rockchip_nand_erase_brom_blks(start, blkcnt);
}

int rockusb_nand_read(struct rockusb_dev_desc *desc, unsigned int lba,
		      unsigned int blkcount, char *buf)
{
	return rockchip_nand_read_brom_blks(buf, blkcount, lba, desc->blksz);
}

int rockusb_nand_write(struct rockusb_dev_desc *desc, unsigned int lba,
		       unsigned int blkcount, char *buf)
{
	return rockchip_nand_write_brom_blks(buf, blkcount, lba, desc->blksz);
}

int rockusb_fill_nand_dev(struct rockusb_dev_desc *desc, char *dev_type,
			  unsigned int dev_index)
{
	struct udevice *dev;
	int ret;

	if(desc->priv == (void *) INIT_COMPLETE_MAGIC)
		return 0;

	ret = uclass_get_device_by_driver(UCLASS_MTD,
					  DM_DRIVER_GET(rk_nandc_brom_v6),
					  &dev);
	if (ret)
		debug("%s: Failed to initialize NAND controller. (error %d)\n",
		      __func__, ret);

	desc->blksz = ROCKCHIP_NAND_BLK_SZ;
	desc->priv = (void *) INIT_COMPLETE_MAGIC;
	desc->rockusb_erase = rockusb_nand_erase;
	desc->rockusb_read = rockusb_nand_read;
	desc->rockusb_write = rockusb_nand_write;

	return 0;
}