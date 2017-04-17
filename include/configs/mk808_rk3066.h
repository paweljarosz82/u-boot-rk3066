/*
 * Copyright (c) 2017 Paweł Jarosz <paweljarosz3691@gmail.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H
/*
 * Using bootrom layout for rockchip-idb* and spl*. Size is nand erase size.
 */
#define MTDPARTS_DEFAULT \
		"mtdparts=rockchip-nand.0:" \
			"4M(idb)," \
			"4M(idb.backup)," \
			"4M(spl)," \
			"4M(spl.backup1)," \
			"4M(spl.backup2)," \
			"4M(spl.backup3)," \
			"4M(spl.backup4)," \
			"4M(u-boot)," \
			"4M(u-boot.backup)," \
			"4M(u-boot-env)," \
			"4M(u-boot-env.backup)," \
			"16M(kernel)," \
			"32M(initrd)," \
			"-(rootfs)"

#define ROCKCHIP_DEVICE_SETTINGS \
	"mtdparts=" MTDPARTS_DEFAULT "\0"

#include <configs/rk3066_common.h>

#endif

