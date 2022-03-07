// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <hang.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <spl.h>
#if CONFIG_IS_ENABLED(BANNER_PRINT)
#include <timestamp.h>
#endif
#include <version.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/kconfig.h>

#if defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD)

#if CONFIG_IS_ENABLED(USB_GADGET)
#include <usb.h>

#if defined(CONFIG_USB_GADGET_DWC2_OTG)
#include <usb/dwc2_udc.h>

static struct dwc2_plat_otg_data otg_data = {
	.rx_fifo_sz	= 512,
	.np_tx_fifo_sz	= 16,
	.tx_fifo_sz	= 128,
};

int board_usb_init(int index, enum usb_init_type init)
{
	ofnode node;
	const char *mode;
	bool matched = false;

	/* find the usb_otg node */
	node = ofnode_by_compatible(ofnode_null(), "snps,dwc2");
	while (ofnode_valid(node)) {
		mode = ofnode_read_string(node, "dr_mode");
		if (mode && strcmp(mode, "otg") == 0) {
			matched = true;
			break;
		}

		node = ofnode_by_compatible(node, "snps,dwc2");
	}
	if (!matched) {
		debug("Not found usb_otg device\n");
		return -ENODEV;
	}
	otg_data.regs_otg = ofnode_get_addr(node);

#ifdef CONFIG_ROCKCHIP_RK3288
	int ret;
	u32 phandle, offset;
	ofnode phy_node;

	ret = ofnode_read_u32(node, "phys", &phandle);
	if (ret)
		return ret;

	node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(node)) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	phy_node = ofnode_get_parent(node);
	if (!ofnode_valid(node)) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	otg_data.phy_of_node = phy_node;
	ret = ofnode_read_u32(node, "reg", &offset);
	if (ret)
		return ret;
	otg_data.regs_phy =  offset +
		(u32)syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
#endif
	return dwc2_udc_probe(&otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	return 0;
}
#endif /* CONFIG_USB_GADGET_DWC2_OTG */

#if defined(CONFIG_USB_DWC3_GADGET) && !defined(CONFIG_DM_USB_GADGET)
#include <dwc3-uboot.h>

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfe800000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.hsphy_mode = USBPHY_INTERFACE_MODE_UTMIW,
};

int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return dwc3_uboot_init(&dwc3_device_data);
}
#endif /* CONFIG_USB_DWC3_GADGET */

#endif /* CONFIG_USB_GADGET */

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_NAND;
}

void board_init_f(ulong dummy)
{
	int ret;

#ifdef CONFIG_DEBUG_UART
	/*
	 * Debug UART can be used from here if required:
	 *
	 * debug_uart_init();
	 * printch('a');
	 * printhex8(0x1234);
	 * printascii("string");
	 */
	debug_uart_init();
	debug("\nspl:debug uart enabled in %s\n", __func__);

#if CONFIG_IS_ENABLED(BANNER_PRINT)
        puts("\nU-Boot USBPLUG" " " PLAIN_VERSION " (" U_BOOT_DATE " - "
             U_BOOT_TIME " " U_BOOT_TZ ")\n");
#endif
#endif

	ret = spl_early_init();
	if (ret) {
		printf("spl_early_init() failed: %d\n", ret);
		hang();
	}
}
#endif

