/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <syscon.h>
#include <asm/arch/clock.h>

static const struct udevice_id rk3066_syscon_ids[] = {
	{ .compatible = "rockchip,rk3066-grf", .data = ROCKCHIP_SYSCON_GRF },
	{ }
};

U_BOOT_DRIVER(syscon_rk3066) = {
	.name = "rk3066_syscon",
	.id = UCLASS_SYSCON,
	.of_match = rk3066_syscon_ids,
};
