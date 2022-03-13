// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2017 Yifeng Zhao <yifeng.zhao@rock-chips.com>
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/read.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <inttypes.h>
#include <nand.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/rknand.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/iopoll.h>

#define BROM_DEFAULT_BLKS	2
#define BROM_DEFAULT_STRENGTH	24

#define NANDC_V6_DEF_TIMEOUT	20000
#define NANDC_V6_READ		0
#define NANDC_V6_WRITE		1

#define	NANDC_REG_V6_FMCTL	0x00
#define	NANDC_REG_V6_FMWAIT	0x04
#define	NANDC_REG_V6_FLCTL	0x08
#define	NANDC_REG_V6_BCHCTL	0x0c
#define	NANDC_REG_V6_DMA_CFG	0x10
#define	NANDC_REG_V6_DMA_BUF0	0x14
#define	NANDC_REG_V6_DMA_BUF1	0x18
#define NANDC_REG_V6_DMA_ST	0x1C
#define	NANDC_REG_V6_BCHST	0x20
#define	NANDC_REG_V6_RANDMZ	0x150
#define	NANDC_REG_V6_VER	0x160
#define	NANDC_REG_V6_INTEN	0x16C
#define	NANDC_REG_V6_INTCLR	0x170
#define	NANDC_REG_V6_INTST	0x174
#define	NANDC_REG_V6_SPARE0	0x200
#define	NANDC_REG_V6_SPARE1	0x230
#define	NANDC_REG_V6_BANK0	0x800
#define	NANDC_REG_V6_SRAM0	0x1000
#define	NANDC_REG_V6_SRAM_SIZE	0x400

#define NANDC_REG_V6_DATA	0x00
#define NANDC_REG_V6_ADDR	0x04
#define NANDC_REG_V6_CMD	0x08

/* FMCTL */
#define NANDC_V6_FM_WP		BIT(8)
#define NANDC_V6_FM_CE_SEL_M	0xFF
#define NANDC_V6_FM_CE_SEL(x)	BIT(x)
#define NANDC_V6_FM_FREADY	BIT(9)

/* FLCTL */
#define NANDC_V6_FL_RST		BIT(0)
#define NANDC_V6_FL_DIR_S	0x1
#define NANDC_V6_FL_XFER_START	BIT(2)
#define NANDC_V6_FL_XFER_EN	BIT(3)
#define NANDC_V6_FL_ST_BUF_S	0x4
#define NANDC_V6_FL_XFER_COUNT	BIT(5)
#define NANDC_V6_FL_ACORRECT	BIT(10)
#define NANDC_V6_FL_XFER_READY	BIT(20)

/* BCHCTL */
#define NAND_V6_BCH_REGION_S	0x5
#define NAND_V6_BCH_REGION_M	0x7

static struct rockchip_nand *rknand;

void rockchip_nand_wait_dev_ready(void)
{
	u32 reg;

	readl_poll_sleep_timeout(rknand->regs + NANDC_REG_V6_FMCTL,
				 reg,
				 reg & NANDC_V6_FM_FREADY,
				 1,
				 NANDC_V6_DEF_TIMEOUT);
}

__weak void rockchip_nand_board_early_init(void) {}

void rockchip_nand_init(void)
{
	rockchip_nand_board_early_init();

	writel(NANDC_V6_FM_WP, rknand->regs + NANDC_REG_V6_FMCTL);
	/* Config default timing 40ns at 150 Mhz NFC clock. */
	writel(0x1081, rknand->regs + NANDC_REG_V6_FMWAIT);
	writel(0, rknand->regs + NANDC_REG_V6_RANDMZ);
	writel(0, rknand->regs + NANDC_REG_V6_DMA_CFG);
	writeb(NAND_CMD_RESET,
	       rknand->regs + NANDC_REG_V6_BANK0 + NANDC_REG_V6_CMD);
	rockchip_nand_wait_dev_ready();
}

void rockchip_nand_select_chip(int chipnr)
{
	u32 reg;

	reg = readl(rknand->regs + NANDC_REG_V6_FMCTL);
	reg &= ~NANDC_V6_FM_CE_SEL_M;
	if (chipnr != -1)
		reg |= 1 << chipnr;
	writel(reg, rknand->regs + NANDC_REG_V6_FMCTL);
}

void rockchip_nand_pio_xfer_start(u8 dir, u8 st_buf)
{
	u32 reg;

	reg = readl(rknand->regs + NANDC_REG_V6_BCHCTL);
	reg = (reg & (~(NAND_V6_BCH_REGION_M << NAND_V6_BCH_REGION_S)));
	writel(reg, rknand->regs + NANDC_REG_V6_BCHCTL);

	reg = (dir << NANDC_V6_FL_DIR_S) | (st_buf << NANDC_V6_FL_ST_BUF_S) |
	      NANDC_V6_FL_XFER_EN | NANDC_V6_FL_XFER_COUNT |
	      NANDC_V6_FL_ACORRECT;
	writel(reg, rknand->regs + NANDC_REG_V6_FLCTL);

	reg |= NANDC_V6_FL_XFER_START;
	writel(reg, rknand->regs + NANDC_REG_V6_FLCTL);
}

int rockchip_nand_wait_pio_xfer_done(void)
{
	u32 reg;

	return readl_poll_sleep_timeout(rknand->regs + NANDC_REG_V6_FLCTL,
					reg,
					reg & NANDC_V6_FL_XFER_READY,
					1,
					NANDC_V6_DEF_TIMEOUT);
}

void rockchip_nand_read_page_op(int page, int col)
{
	void __iomem *bank_base = rknand->regs + NANDC_REG_V6_BANK0;

	writeb(NAND_CMD_READ0, bank_base + NANDC_REG_V6_CMD);
	writeb(col, bank_base + NANDC_REG_V6_ADDR);
	writeb(col >> 8, bank_base + NANDC_REG_V6_ADDR);
	writeb(page, bank_base + NANDC_REG_V6_ADDR);
	writeb(page >> 8, bank_base + NANDC_REG_V6_ADDR);
	writeb(page >> 16, bank_base + NANDC_REG_V6_ADDR);
	writeb(NAND_CMD_READSTART, bank_base + NANDC_REG_V6_CMD);
}

int rockchip_nand_read_page(unsigned int page, uint8_t *buf, u8 ecc_steps)
{
	void __iomem *sram_base = rknand->regs + NANDC_REG_V6_SRAM0;
	int ret;

	rockchip_nand_select_chip(0);
	rockchip_nand_read_page_op(page, 0);
	rockchip_nand_wait_dev_ready();
	rockchip_nand_pio_xfer_start(NANDC_V6_READ, 0);

	for (int step = 0; step < ecc_steps; step++) {
		int data_off = step * rknand->eccstep;
		u8 *data = buf + data_off;

		ret = rockchip_nand_wait_pio_xfer_done();
		if (ret)
			return ret;

		if ((step + 1) < ecc_steps)
			rockchip_nand_pio_xfer_start(NANDC_V6_READ, (step + 1) & 0x1);

		memcpy_fromio(data, sram_base + rknand->eccstep * (step & 0x1),
			      NANDC_REG_V6_SRAM_SIZE);
	}

	rockchip_nand_select_chip(-1);

	return 0;
}

void rockchip_nand_write_page_op_begin(int page, int col)
{
	void __iomem *bank_base = rknand->regs + NANDC_REG_V6_BANK0;

	rockchip_nand_select_chip(0);

	writeb(NAND_CMD_SEQIN, bank_base + NANDC_REG_V6_CMD);
	writeb(col, bank_base + NANDC_REG_V6_ADDR);
	writeb(col >> 8, bank_base + NANDC_REG_V6_ADDR);
	writeb(page, bank_base + NANDC_REG_V6_ADDR);
	writeb(page >> 8, bank_base + NANDC_REG_V6_ADDR);
	writeb(page >> 16, bank_base + NANDC_REG_V6_ADDR);
}

void rockchip_nand_write_page_op_end(void)
{
	void __iomem *bank_base = rknand->regs + NANDC_REG_V6_BANK0;

	writeb(NAND_CMD_PAGEPROG, bank_base + NANDC_REG_V6_CMD);

	rockchip_nand_wait_dev_ready();
	rockchip_nand_select_chip(-1);
}

int rockchip_nand_write_page(unsigned int page, u8 *buf, u8 ecc_steps)
{
	void __iomem *sram_base = rknand->regs + NANDC_REG_V6_SRAM0;
	int ret;
	u8 *data;
	u32 data_off;

	rockchip_nand_write_page_op_begin(page, 0);

	memcpy_toio(sram_base, buf, rknand->eccstep);
	writel(0, rknand->regs + NANDC_REG_V6_SPARE0);

	for (int step = 1; step <= ecc_steps; step++) {
		rockchip_nand_pio_xfer_start(NANDC_V6_WRITE, (step - 1) & 0x1);

		data_off = step * rknand->eccstep;
		data = buf + data_off;

		if (step < ecc_steps) {
			memcpy_toio(sram_base + NANDC_REG_V6_SRAM_SIZE *
				    (step & 1), data, rknand->eccstep);
			if (step & 1)
				writel(0, rknand->regs + NANDC_REG_V6_SPARE1);
			else
				writel(0, rknand->regs + NANDC_REG_V6_SPARE0);
		}

		ret = rockchip_nand_wait_pio_xfer_done();
		if (ret)
			return ret;
	}
	rockchip_nand_write_page_op_end();

	return 0;
}

u32 rockchip_nand_erase_brom_blks(u32 start, u32 blkcnt)
{
	void __iomem *bank_base = rknand->regs + NANDC_REG_V6_BANK0;

	rockchip_nand_select_chip(0);

	for (int page = start; page < start + blkcnt * rknand->pagesperblock;
	     page += rknand->pagesperblock) {
		writeb(NAND_CMD_ERASE1, bank_base + NANDC_REG_V6_CMD);
		writeb(page, bank_base + NANDC_REG_V6_ADDR);
		writeb(page >> 8, bank_base + NANDC_REG_V6_ADDR);
		writeb(page >> 16, bank_base + NANDC_REG_V6_ADDR);
		writeb(NAND_CMD_ERASE2, bank_base + NANDC_REG_V6_CMD);
		rockchip_nand_wait_dev_ready();

		debug("%s: %x\n", __func__, page);
	}

	rockchip_nand_select_chip(-1);

	return blkcnt;
}

u32 rockchip_nand_read_brom_blks(u8 *buf, u32 blkcnt, u32 lba, u32 blk_size)
{
	u8 steps = 2;
	u32 buf_size = blkcnt * blk_size;
	u32 bytesperpage = steps * rknand->eccstep;

	/*
	 * Rockchip uses lba 0x40 as signal to write bootloader
	 * and increments by it by 8 every cycle. Also reads two
	 * 2KB chunks every cycle.
	 * But bootrom starts to search for bootloader from page 0.
	 */
	u32 page_off = (lba - 0x40) / 4;

	for (int page = 0; page < buf_size / bytesperpage; page++) {
		rockchip_nand_read_page(page + page_off,
					buf + bytesperpage * page, steps);
		debug("%s: reading page %x, size: %x, lba: %x\n",
		      __func__, page + page_off, buf_size, lba);
	}

	return blkcnt;
}

u32 rockchip_nand_write_brom_blks(u8 *buf, u32 blkcnt, u32 lba, u32 blk_size)
{
	u8 steps = 2;
	u32 buf_size = blkcnt * blk_size;
	u32 bytesperpage = steps * rknand->eccstep;

	/*
	 * Rockchip uses lba 0x40 as signal to write bootloader
	 * and increments by it by 8 every cycle. Also writes two
	 * 2KB chunks every cycle.
	 * But bootrom starts to search for bootloader from page 0.
	 */
	u32 page_off = (lba - 0x40) / 4;

	/* We only support lba >= 0x40 and page number less than pages in block */
	if (lba < 0x40 || page_off > rknand->pagesperblock)
		return 0;

	for (int page = 0; page < buf_size / bytesperpage; page++) {
		/* Fill all brom blocks */
		for (int bromblock = 0; bromblock < rknand->bromblocks; bromblock++) {
			/* Erase block before writing to first page in block */
			if ((page + page_off) % rknand->pagesperblock == 0)
				rockchip_nand_erase_brom_blks(page + page_off +
							      rknand->pagesperblock * bromblock, 1);

			rockchip_nand_write_page(page + page_off +
						 rknand->pagesperblock * bromblock,
						 buf + bytesperpage * page, steps);
			debug("%s: writing page %x, size: %x, lba: %x\n",
			      __func__, page + page_off + rknand->pagesperblock * bromblock,
			      buf_size, lba);
		}
	}

	return blkcnt;
}

void rockchip_nand_read_id(u8 *id, int len)
{
	void __iomem *bank_base = rknand->regs + NANDC_REG_V6_BANK0;

	rockchip_nand_select_chip(0);

	writeb(NAND_CMD_READID, bank_base + NANDC_REG_V6_CMD);
	writeb(0x0, bank_base + NANDC_REG_V6_ADDR);
	udelay(1);

	debug("%s: NAND ID:", __func__);

	for (int i = 0; i < len; i++) {
		id[i] = readb(bank_base);
		debug(" %x", id[i]);
	}
	debug("\n");

	rockchip_nand_select_chip(-1);
}

int rockchip_nand_get_chip_info(struct udevice *dev)
{
	struct nand_flash_dev *type;
	u8 id[8];
	ofnode node;

	rockchip_nand_read_id(id, 8);

	for (type = nand_flash_ids; type->name; type++)
		if (type->id_len)
			if (!strncmp((char *)type->id, (char *)id, type->id_len))
				break;

	if (!type->name)
		return -ENODEV;

	rknand->chipsize = type->chipsize;
	rknand->eccstep = type->ecc.step_ds;
	rknand->eccstrength = type->ecc.strength_ds;
	rknand->erasesize = type->erasesize;
	rknand->oobsize = type->oobsize;
	rknand->options = type->options;
	rknand->pagesize = type->pagesize;
	rknand->pagesperblock = type->erasesize / type->pagesize;

	node = dev_read_subnode(dev, "nand");
	if (!ofnode_valid(node)) {
		debug("%s: Nand subnode not found\n", __func__);
		return -ENODEV;
	}

	rknand->bromblocks = ofnode_read_u32_default(node,
						     "rockchip,boot-blks",
						     BROM_DEFAULT_BLKS);
	rknand->bromstrength =
			ofnode_read_u32_default(node,
						"rockchip,boot-ecc-strength",
						BROM_DEFAULT_STRENGTH);

	debug("NAND: %s\n", type->name);
	debug("\tpagesize: %d B\n", type->pagesize);
	debug("\tchipsize: %d GB\n", type->chipsize);
	debug("\terasesize: %d B\n", type->erasesize);
	debug("\toptions: 0x%x\n", type->options);
	debug("\toobsize: %d B\n", type->oobsize);
	debug("\teccstrength: %d bits\n", type->ecc.strength_ds);
	debug("\teccstep: %d B\n", type->ecc.step_ds);
	debug("\tbromblocks: %d\n", rknand->bromblocks);
	debug("\tbromstrength: %d\n", rknand->bromstrength);

	return 0;
}

int rockchip_nand_hw_ecc_setup(void)
{
	u32 reg;

	switch (rknand->bromstrength) {
	case 24:
		reg = 0x00000010;
		break;
	case 16:
		reg = 0x00000000;
		break;
	default:
		debug("%s: Invalid bootrom block strength: %d",
		      __func__, rknand->bromstrength);
		return -EINVAL;
	}

	writel(reg, rknand->regs + NANDC_REG_V6_BCHCTL);

	return 0;
}

int rockchip_nand_enable_clks(struct udevice *dev)
{
	int ret;

	rknand->ahb_clk = devm_clk_get(dev, "ahb");
	if (IS_ERR(rknand->ahb_clk)) {
		debug("%s: no ahb clk\n", __func__);
		return -EINVAL;
	}

	ret = clk_prepare_enable(rknand->ahb_clk);
	if (ret) {
		debug("%s: failed to enable ahb clk\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static const struct udevice_id rockchip_nandc_ids[] = {
	{ .compatible = "rockchip,rk2928-nfc" },
	{ }
};

int rockchip_nandc_probe(struct udevice *dev)
{
	fdt_addr_t regs;
	int ret = 0;

	rknand = dev_get_priv(dev);

	regs = dev_read_addr(dev);
	if (!regs) {
		debug("%s: Nand address not found\n", __func__);
		return -ENODEV;
	}

	rknand->regs = (void *)regs;

	ret = rockchip_nand_enable_clks(dev);
	if (ret)
		return ret;

	rockchip_nand_init();

	ret = rockchip_nand_get_chip_info(dev);
	if (ret) {
		debug(" %s: Failed to get chip info\n", __func__);
		return ret;
	}

	ret = rockchip_nand_hw_ecc_setup();
	if (ret) {
		debug(" %s: Failed to config hw ecc strength\n", __func__);
		return ret;
	}

	return 0;
}

U_BOOT_DRIVER(rk_nandc_brom_v6) = {
	.name           = "rk_nandc_brom_v6",
	.id             = UCLASS_MTD,
	.of_match       = rockchip_nandc_ids,
	.probe          = rockchip_nandc_probe,
	.priv_auto      = sizeof(struct rockchip_nand),
};

void nand_init(void) {}

void nand_deselect(void) {}

int nand_spl_load_image(u32 offs, unsigned int size, void *dst)
{
	return -ENODEV;
}
