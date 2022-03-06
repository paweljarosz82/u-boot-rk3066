// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Pawel Jarosz <paweljarosz3691@gmail.com>
 */

#include <common.h>
#include <console.h>
#include <g_dnl.h>
#include <spl.h>
#include <usb.h>
#include <asm/arch-rockchip/f_rockusb.h>
#include <asm/spl.h>

int rockusb_start(struct spl_image_info *spl_image,
		  struct spl_boot_device *bootdev)
{
	int controller_index = 0;
	int dev_index;
	char *devtype;
	int ret;

	switch (bootdev->boot_device) {
	case BOOT_DEVICE_MMC1:
		devtype = "mmc";
		dev_index = 0;
		break;
	case BOOT_DEVICE_MMC2:
		devtype = "mmc";
		dev_index = 1;
		break;
	case BOOT_DEVICE_NAND:
		devtype = "nand";
		dev_index = 0;
		break;
	default:
		printf("ROCKUSB invalid boot device!\n");
		return -ENOENT;
	}

	rockusb_dev_init(devtype, dev_index);

	ret = usb_gadget_initialize(controller_index);
	if (ret) {
		printf("USB init failed: %d\n", ret);
		return ret;
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_rockusb");
	if (ret)
		return ret;

	if (!g_dnl_board_usb_cable_connected()) {
		puts("\rUSB cable not detected, exiting.\n");
		ret = -ENODEV;
		goto exit;
	}

	while (1) {
		if (g_dnl_detach())
			break;
		if (ctrlc())
			break;
		usb_gadget_handle_interrupts(controller_index);
	}
	ret = 0;

exit:
	g_dnl_unregister();
	g_dnl_clear_detach();
	usb_gadget_release(controller_index);

	return ret;
}

SPL_LOAD_IMAGE_METHOD("ROCKUSB MMC1", 0, BOOT_DEVICE_MMC1, rockusb_start);
SPL_LOAD_IMAGE_METHOD("ROCKUSB MMC2", 0, BOOT_DEVICE_MMC2, rockusb_start);
SPL_LOAD_IMAGE_METHOD("ROCKUSB NAND", 0, BOOT_DEVICE_NAND, rockusb_start);
