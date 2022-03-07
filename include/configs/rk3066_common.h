/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2015 Google, Inc
 */

#ifndef __CONFIG_RK3066_COMMON_H
#define __CONFIG_RK3066_COMMON_H

#define CONFIG_SYS_CACHELINE_SIZE	64

#include <asm/arch-rockchip/hardware.h>
#include "rockchip-common.h"

#define CONFIG_USB_GADGET_MANUFACTURER "Rockchip"
#define CONFIG_USB_GADGET_PRODUCT_NUM 0x300a
#define CONFIG_USB_GADGET_VENDOR_NUM 0x2207


#define CONFIG_SKIP_LOWLEVEL_INIT_ONLY
#define CONFIG_SYS_CBSIZE		256

#define CONFIG_SYS_INIT_SP_ADDR		0x61000000

#define CONFIG_ROCKCHIP_MAX_INIT_SIZE	(0x10000 - 0xC00)

#define CONFIG_IRAM_BASE		0x10080000

/* spl size max 200k */
#define CONFIG_SPL_MAX_SIZE		0x32000

#define CONFIG_SYS_SDRAM_BASE		0x60000000
#define CONFIG_NR_DRAM_BANKS		1
#define SDRAM_BANK_SIZE			(1024UL << 20UL)
#define SDRAM_MAX_SIZE			CONFIG_NR_DRAM_BANKS * SDRAM_BANK_SIZE

#ifdef CONFIG_TPL_BUILD
#define CONFIG_SPL_STACK	0x1008FFFF
#ifdef CONFIG_HAS_VBAR
#undef CONFIG_HAS_VBAR
#endif
#else
#define CONFIG_SPL_STACK	0x78000000
#endif

#ifndef CONFIG_SPL_BUILD
/* usb otg */

/* usb host support */
#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x60000000\0" \
	"pxefile_addr_r=0x60100000\0" \
	"fdt_addr_r=0x61f00000\0" \
	"kernel_addr_r=0x62000000\0" \
	"ramdisk_addr_r=0x64000000\0"

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0x6fffffff\0" \
	"initrd_high=0x6fffffff\0" \
	"partitions=" PARTS_DEFAULT \
	ENV_MEM_LAYOUT_SETTINGS \
	ROCKCHIP_DEVICE_SETTINGS \
	BOOTENV

#endif /* CONFIG_SPL_BUILD */

#endif
