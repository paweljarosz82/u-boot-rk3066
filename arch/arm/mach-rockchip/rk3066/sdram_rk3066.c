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
#include <linux/err.h>

DECLARE_GLOBAL_DATA_PTR;

struct dram_info {
	struct ram_info info;
	struct rk3188_pmu *pmu;
};

size_t sdram_size_mb(struct rk3188_pmu *pmu)
{
	u32 rank, col, bk, cs0_row, cs1_row, bw, row_3_4;
	size_t chipsize_mb = 0;
	size_t size_mb = 0;
	u32 ch;
	u32 sys_reg = readl(&pmu->sys_reg[2]);
	u32 chans;

	chans = 1 + ((sys_reg >> SYS_REG_NUM_CH_SHIFT) & SYS_REG_NUM_CH_MASK);

	for (ch = 0; ch < chans; ch++) {
		rank = 1 + (sys_reg >> SYS_REG_RANK_SHIFT(ch) &
			SYS_REG_RANK_MASK);
		col = 9 + (sys_reg >> SYS_REG_COL_SHIFT(ch) & SYS_REG_COL_MASK);
		bk = 3 - ((sys_reg >> SYS_REG_BK_SHIFT(ch)) & SYS_REG_BK_MASK);
		cs0_row = 13 + (sys_reg >> SYS_REG_CS0_ROW_SHIFT(ch) &
				SYS_REG_CS0_ROW_MASK);
		cs1_row = 13 + (sys_reg >> SYS_REG_CS1_ROW_SHIFT(ch) &
				SYS_REG_CS1_ROW_MASK);
		bw = (2 >> ((sys_reg >> SYS_REG_BW_SHIFT(ch)) &
			SYS_REG_BW_MASK));
		row_3_4 = sys_reg >> SYS_REG_ROW_3_4_SHIFT(ch) &
			SYS_REG_ROW_3_4_MASK;
		chipsize_mb = (1 << (cs0_row + col + bk + bw - 20));

		if (rank > 1)
			chipsize_mb += chipsize_mb >>
				(cs0_row - cs1_row);
		if (row_3_4)
			chipsize_mb = chipsize_mb * 3 / 4;
		size_mb += chipsize_mb;
	}

	/* there can be no more than 2gb of memory */
	size_mb = min(size_mb, 0x80000000 >> 20);

	return size_mb;
}

static int rk3066_dmc_probe(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->pmu = syscon_get_first_range(ROCKCHIP_SYSCON_PMU);

	priv->info.base = CONFIG_SYS_SDRAM_BASE;
	priv->info.size = sdram_size_mb(priv->pmu) << 20;

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
