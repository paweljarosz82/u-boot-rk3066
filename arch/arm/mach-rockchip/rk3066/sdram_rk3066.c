/*
 * (C) Copyright 2015 Google, Inc
 * Copyright 2014 Rockchip Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0
 *
 * Adapted from the very similar rk3288 ddr init.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dt-structs.h>
#include <errno.h>
#include <ram.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_rk3066.h>
#include <asm/arch/ddr_rk3188.h>
#include <asm/arch/grf_rk3066.h>
#include <asm/arch/pmu_rk3188.h>
#include <asm/arch/sdram.h>
#include <asm/arch/sdram_common.h>
#include <linux/err.h>

DECLARE_GLOBAL_DATA_PTR;

struct dram_info {
	struct ram_info info;
	struct rk3188_pmu *pmu;
};

static int rk3066_dmc_probe(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->pmu = syscon_get_first_range(ROCKCHIP_SYSCON_PMU);

	priv->info.base = CONFIG_SYS_SDRAM_BASE;
	priv->info.size = rockchip_sdram_size((phys_addr_t)&priv->pmu->sys_reg[2]);

	return 0;
}

static int rk3066_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops rk3066_dmc_ops = {
	.get_info = rk3066_dmc_get_info,
};

static const struct udevice_id rk3066_dmc_ids[] = {
	{ .compatible = "rockchip,rk3066-dmc" },
	{ }
};

U_BOOT_DRIVER(dmc_rk3066) = {
	.name = "rockchip_rk3066_dmc",
	.id = UCLASS_RAM,
	.of_match = rk3066_dmc_ids,
	.ops = &rk3066_dmc_ops,
	.probe = rk3066_dmc_probe,
	.priv_auto_alloc_size = sizeof(struct dram_info),
};
