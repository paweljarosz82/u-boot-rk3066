// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Pawel Jarosz <paweljarosz3691@gmail.com>
 */

#include <common.h>
#include <linux/usb/composite.h>
#include <asm/arch-rockchip/f_rockusb.h>

struct rockusb_dev_desc *rockusb_get_dev(char *dev_type,
					 unsigned int dev_index)
{
	static struct rockusb_dev_desc *desc;
	int ret = 0;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return ERR_PTR(-ENOMEM);

	if (strcmp(dev_type, "mmc") == 0)
		ret = rockusb_fill_mmc_dev(desc, dev_type, dev_index);

	else if (strcmp(dev_type, "nand") == 0)
		ret = rockusb_fill_nand_dev(desc, dev_type, dev_index);

	if (ret)
		goto error;
	else
		return desc;

error:
	kfree(desc);

	return ERR_PTR(ret);
}
