/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/ddr_rk3188.h>
#include <asm/arch/grf_rk3066.h>
#include <asm/arch/pmu_rk3188.h>

DECLARE_GLOBAL_DATA_PTR;

#define RK3066_TIMER_CONTROL	0x8
#define GRF_BASE	0x20008000

void board_init_f(ulong dummy)
{
	/* Enable early UART on the RK3066 */

	struct rk3066_grf * const grf = (void *)GRF_BASE;

	rk_clrsetreg(&grf->gpio1b_iomux,
		     GPIO1B1_MASK << GPIO1B1_SHIFT |
		     GPIO1B0_MASK << GPIO1B0_SHIFT,
		     GPIO1B1_UART2_SOUT << GPIO1B1_SHIFT |
		     GPIO1B0_UART2_SIN << GPIO1B0_SHIFT);

	debug_uart_init();

	printch('T');
	printch('P');
	printch('L');
	printch('\n');

	/* Enable Timer0 */
	rk_clrsetreg(CONFIG_SYS_TIMER_BASE + RK3066_TIMER_CONTROL, 0x1, 0x1);

	sdram_initialise();

	back_to_bootrom();
}
