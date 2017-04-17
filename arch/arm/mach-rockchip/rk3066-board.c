/*
 * (C) Copyright 2017 Paweł Jarosz <paweljarosz3691@gmail.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <clk.h>
#include <common.h>
#include <asm/arch/periph.h>
#include <asm/io.h>
#include <asm/arch/uart.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/clock/rk3066a-cru.h>

DECLARE_GLOBAL_DATA_PTR;

static int setup_arm_clock(void)
{
	struct udevice *dev;
	struct clk clk;
	int ret;

	ret = rockchip_get_clk(&dev);
	if (ret)
		return ret;

	clk.id = PLL_APLL;
	ret = clk_request(dev, &clk);
	if (ret < 0)
		return ret;

	ret = clk_set_rate(&clk, 1416000000);

	clk_free(&clk);
	return ret;
}

int board_init(void)
{
	setup_arm_clock();
	return 0;
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

int print_cpuinfo (void)
{	
	printf("CPU:   Rockchip RK3066\n");
	return 0;
}

#if defined(CONFIG_USB_GADGET) && defined(CONFIG_USB_GADGET_DWC2_OTG)
#include <usb.h>
#include <usb/dwc2_udc.h>

static struct dwc2_plat_otg_data rk3066_otg_data = {
	.rx_fifo_sz	= 275,
	.np_tx_fifo_sz	= 16,
	.tx_fifo_sz	= 256,
};

int board_usb_init(int index, enum usb_init_type init)
{
	int node, phy_node;
	const char *mode;
	bool matched = false;
	const void *blob = gd->fdt_blob;
	u32 grf_phy_offset;

	/* find the usb_otg node */
	node = fdt_node_offset_by_compatible(blob, -1,
					"rockchip,rk3288-usb");

	while (node > 0) {
		mode = fdt_getprop(blob, node, "dr_mode", NULL);
		if (mode && strcmp(mode, "otg") == 0) {
			matched = true;
			break;
		}

		node = fdt_node_offset_by_compatible(blob, node,
					"rockchip,rk3288-usb");
	}
	if (!matched) {
		debug("Not found usb_otg device\n");
		return -ENODEV;
	}
	rk3066_otg_data.regs_otg = fdtdec_get_addr(blob, node, "reg");

	node = fdtdec_lookup_phandle(blob, node, "phys");
	if (node <= 0) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	phy_node = fdt_parent_offset(blob, node);
	if (phy_node <= 0) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	rk3066_otg_data.phy_of_node = phy_node;
	grf_phy_offset = fdtdec_get_addr(blob, node, "reg");

	node = fdt_node_offset_by_compatible(blob, -1,
					"rockchip,rk3066-grf");
	if (node <= 0) {
		debug("Not found grf device\n");
		return -ENODEV;
	}
	rk3066_otg_data.regs_phy = grf_phy_offset +
				fdtdec_get_addr(blob, node, "reg");

	return dwc2_udc_probe(&rk3066_otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	return 0;
}
#endif
