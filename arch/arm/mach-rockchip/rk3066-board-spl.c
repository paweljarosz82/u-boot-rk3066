/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <clk.h>
#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <fdtdec.h>
#include <led.h>
#include <malloc.h>
#include <ram.h>
#include <spl.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/clock.h>
#include <asm/arch/hardware.h>
#include <asm/arch/periph.h>
#include <asm/arch/pmu_rk3188.h>
#include <asm/arch/sdram.h>
#include <asm/arch/timer.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <dm/test.h>
#include <dm/util.h>
#include <power/regulator.h>
#include <syscon.h>

DECLARE_GLOBAL_DATA_PTR;

u32 spl_boot_device(void)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	const void *blob = gd->fdt_blob;
	struct udevice *dev;
	const char *bootdev;
	int node;
	int ret;

	bootdev = fdtdec_get_config_string(blob, "u-boot,boot0");
	debug("Boot device %s\n", bootdev);
	if (!bootdev)
		goto fallback;

	node = fdt_path_offset(blob, bootdev);
	if (node < 0) {
		debug("node=%d\n", node);
		goto fallback;
	}
	ret = device_get_global_by_of_offset(node, &dev);
	if (ret) {
		debug("device at node %s/%d not found: %d\n", bootdev, node,
		      ret);
		goto fallback;
	}
	debug("Found device %s\n", dev->name);
	switch (device_get_uclass_id(dev)) {
	case UCLASS_SPI_FLASH:
		return BOOT_DEVICE_SPI;
	case UCLASS_MMC:
		return BOOT_DEVICE_MMC1;
	default:
		debug("Booting from device uclass '%s' not supported\n",
		      dev_get_uclass_name(dev));
	}

fallback:
#endif
	return BOOT_DEVICE_MMC1;
}

u32 spl_boot_mode(const u32 boot_device)
{
	return MMCSD_MODE_FS;
}

static int setup_arm_clock(void)
{
	struct udevice *dev;
	struct clk clk;
	int ret;

	ret = rockchip_get_clk(&dev);
	if (ret)
		return ret;

	clk.id = CLK_ARM;
	ret = clk_request(dev, &clk);
	if (ret < 0)
		return ret;

	ret = clk_set_rate(&clk, 600000000);

	clk_free(&clk);
	return ret;
}

void board_init_f(ulong dummy)
{
	struct udevice *pinctrl, *dev;
	int ret;

	debug_uart_init();

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	/* Enable Timer0 */
	rk_clrsetreg(CONFIG_SYS_TIMER_BASE + 0x8, 0x1, 0x1);

	ret = rockchip_get_clk(&dev);
	if (ret) {
		debug("CLK init failed: %d\n", ret);
		return;
	}

	ret = uclass_get_device(UCLASS_PINCTRL, 0, &pinctrl);
	if (ret) {
		debug("Pinctrl init failed: %d\n", ret);
		return;
	}

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return;
	}

	setup_arm_clock();
}

void spl_board_init(void)
{
	struct udevice *pinctrl;
	int ret;

	ret = uclass_get_device(UCLASS_PINCTRL, 0, &pinctrl);
	if (ret) {
		debug("%s: Cannot find pinctrl device\n", __func__);
		goto err;
	}

#ifdef CONFIG_SPL_MMC_SUPPORT
	ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_SDCARD);
	if (ret) {
		debug("%s: Failed to set up SD card\n", __func__);
		goto err;
	}
#endif

	/* Enable debug UART */
	ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_UART_DBG);
	if (ret) {
		debug("%s: Failed to set up console UART\n", __func__);
		goto err;
	}

	preloader_console_init();

	return;

err:
	printf("spl_board_init: Error %d\n", ret);

	/* No way to report error here */
	hang();
}
