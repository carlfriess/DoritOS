/**
 * \file
 * \brief User level serial port driver.
 */

/*
 * Copyright (c) 2007, 2008, 2012, 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, CAB F.78, Universitaetstr. 6, CH-8092 Zurich,
 * Attn: Systems Group.
 */

#ifndef _USER_SERIAL_H_
#define _USER_SERIAL_H_

#include <aos/aos.h>
#include <stdint.h>

#define SERIAL_PORTBASE_INVALID 0xffffffff
#define SERIAL_IRQ_INVALID      0xffffffff
#define SERIAL_MEMBASE_INVALID  0xffffffffffffffffULL
#define UART1_IRQ (32+72)
#define UART2_IRQ (32+73)
#define UART3_IRQ (32+74)
#define UART4_IRQ (32+70)


/**
 * Blocking write to serial port.
 */
void serial_write(uint8_t *buf, size_t len);

/**
 * Initialize UART. Device frame must be mapped
 * at address vbase.
 */
errval_t serial_init(lvaddr_t vbase, uint32_t irq);

/**
 * This is called whenever input is ready. The client of this
 * library must implement this function.
 */ 
void serial_input(uint8_t *buf, size_t len);

#endif
