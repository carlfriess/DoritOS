/**
 * \file lib/omap_timer/timer.h
 * \brief omap global timer
 */

/*
 * Copyright (c) 2013, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef __OMAP_TIMER_H
#define __OMAP_TIMER_H

errval_t omap_timer_init(void);
uint64_t omap_timer_tsc_per_sec(void);
void omap_timer_ctrl(bool enable);
uint64_t omap_timer_read(void);

static inline uint64_t omap_timer_to_ms(uint64_t time)
{
    return (time * 1000.00) / omap_timer_tsc_per_sec();
}

#endif // __OMAP_TIMER_H
