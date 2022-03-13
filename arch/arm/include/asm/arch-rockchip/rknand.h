/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2022 Pawel Jarosz <paweljarosz3691@gmail.com>
 */

#ifndef _RKNAND_H_
#define _RKNAND_H_

/* BROM nand private struct */
struct rockchip_nand {
	void __iomem *regs;		/* Nand registers */
	u32 bromblocks;			/* Number of bootrom blocks in dts */
	u32 bromstrength;		/* BROM blocks strength (16 or 24 bits) */
	struct clk *ahb_clk;	/* Nand controller clock */
	struct udevice *dev;	/* Nand device */
	u32 pagesize;			/* Chip pagesize */
	u32 pagesperblock;		/* Chip pages per erase block */
	u32 chipsize;			/* Chip size */
	u32 erasesize;			/* Chip erase size */
	u32 options;			/* Chip nand options */
	u32 oobsize;			/* Chip OOB area size */
	u32 eccstrength;		/* Chip ecc strength */
	u32 eccstep;			/* Chip ecc step */
};

/* call back function to handle erase lba rockusb command */
u32 rockchip_nand_erase_brom_blks(u32 start, u32 blkcnt);

/* call back function to handle read lba rockusb command */
u32 rockchip_nand_read_brom_blks(u8 *buf, u32 blkcnt, u32 lba, u32 blk_size);

/* call back function to handle write lba rockusb command */
u32 rockchip_nand_write_brom_blks(u8 *buf, u32 blkcnt, u32 lba, u32 blk_size);

#endif /* _RKNAND_H_ */
