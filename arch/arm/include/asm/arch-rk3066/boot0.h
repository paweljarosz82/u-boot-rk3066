/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __ASM_ARCH_BOOT0_H__
#define __ASM_ARCH_BOOT0_H__

#if defined(CONFIG_TPL_BUILD)
	/*
	 * We need to add 4 bytes of space for the 'RK30' at the
	 * beginning of the executable.
	 */
	b 1f	 /* if overwritten, entry-address is at the next word */
1:
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_EARLYRETURN_TO_BROM)
    /*
     * Bootrom booting first stage loader (TPL) on RK3066 requires two
     * returns to bootrom. First at the beginning of the execution of
     * the first 2KB chunk of the loader. Second after first stage
     * loader finishes execution and RAM setup.
     *
     * But when first stage loader is loaded by "rkflashtool l" or
     * or by "rkdeveloptool db", bootrom only requires one return to
     * bootrom after full execution of TPL containing DDR init code.
     *
     * TPL can detect current mode by comparing value of two byte address
     * in SRAM 0x10080258 with value 0x471. If false, TPL loaded from
     * NAND, SPI, EMMC or UART0. If true, it's USB OTG "rkflashtool l" mode
     * and we can skip first return to bootrom.
     */
	ldr	r0, =0x10080258
	ldrh	r0, [r0]
	ldr	r1, =0x471       /* 0x471 == "rkflashtool l" mode */
	cmp	r0, r1           /* check current boot mode. If true skip first */
	beq	reset            /* return to BROM. If false do regular bootup */
	adr     r3, entry_counter
	ldr	r0, [r3]
	cmp	r0, #1           /* check if entry_counter == 1 */
	beq	reset            /* regular bootup */
	add     r0, #1
	str	r0, [r3]         /* increment the entry_counter in memory */
	mov     r0, #0       /* return 0 to the BROM to signal 'OK' */
	bx	lr               /* return control to the BROM */
entry_counter:
	.word   0
#endif

#if defined(CONFIG_SPL_BUILD)
	/* U-Boot proper of armv7 do not need this */
	b reset
#endif

	.align(5), 0x0
_start:
	ARM_VECTORS

#endif
