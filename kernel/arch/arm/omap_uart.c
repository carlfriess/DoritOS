/**
 * \file
 * \brief Kernel serial driver for the OMAP44xx UARTs.  
 */

/*
 * Copyright (c) 2012-2015, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <omap_uart.h>
#include <dev/omap/omap44xx_uart3_dev.h>
#include <kernel.h>
#include <arm.h>
#include <paging_kernel_arch.h>
#include <platform.h>
#include <serial.h>

//
// Serial console and debugger interfaces
//

#define MSG(port, format, ...) \
    printk( LOG_NOTE, "OMAP serial[%d]: "format, port, ## __VA_ARGS__ )

/*
 * Initialize UARTs before the MMU is on.
 */
errval_t serial_early_init(unsigned port)
{
    /* Stripped for AOS m0. */
    return SYS_ERR_OK;
}

/**
 * \brief Prints a single character to a serial port. 
 */
void serial_putchar(unsigned port, char c)
{
    /* Stripped for AOS m0. */
}

/** 
 * \brief Reads a single character from the default serial port.
 */
char serial_getchar(unsigned port)
{
    /* Stripped for AOS m0. */
    return 0;
}
