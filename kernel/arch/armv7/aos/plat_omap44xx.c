/*
 * Reference implementation of AOS milestone 0, on the Pandaboard.
 */

/*
 * Copyright (c) 2009-2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <kernel.h>

#include <assert.h>
#include <bitmacros.h>
#include <maps/omap44xx_map.h>
#include <paging_kernel_arch.h>
#include <platform.h>
#include <serial.h>

#define MSG(format, ...) printk( LOG_NOTE, "OMAP44xx: "format, ## __VA_ARGS__ )

void blink_leds(void);
void init_leds(void);

/* RAM starts at 2G (2 ** 31) on the Pandaboard */
lpaddr_t phys_memory_start= GEN_ADDR(31);

/*** Serial port ***/

unsigned serial_console_port= 2;

errval_t
serial_init(unsigned port, bool initialize_hw) {
    /* XXX - You'll need to implement this, but it's safe to ignore the
     * parameters. */

    return SYS_ERR_OK;
};

void
serial_putchar(unsigned port, char c) {
    /* XXX - You'll need to implement this, but it's safe to ignore the
     * port parameter. */
    
    volatile char *lsr = (char *)0x48020014;
    char *thr = (char *)0x48020000;

    while(!(*lsr & 0x20));
    *thr = c;
}

__attribute__((noreturn))
char
serial_getchar(unsigned port) {

    init_leds();

    long *gpio1_dataout = (long *)0x4A31013C;
    long *gpio4_dataout = (long *)0x4805913C;
    volatile char *lsr = (char *) 0x48020014;
    volatile char *rhr = (char *)0x48020000;

    bool led1_toggle = false;
    bool led2_toggle = false;

    while(1) {
        while(!(*lsr & 0x1));
        switch(*rhr){
            case '1':
                if(led2_toggle){
                    *gpio4_dataout = *gpio4_dataout & 0xFFFFBFFF;
                } else {
                    *gpio4_dataout = *gpio4_dataout | 0x4000;
                }
                led2_toggle = !led2_toggle;
                break;
            case '2':
                if(led1_toggle){
                    *gpio1_dataout = *gpio1_dataout & 0xFFFFFEFF;
                } else {
                    *gpio1_dataout = *gpio1_dataout | 0x100;
                }
                led1_toggle = !led1_toggle;
                break;
        }
    }



}

/*** LED flashing ***/

__attribute__((noreturn))
void
blink_leds(void) {
    
    init_leds();

    long *gpio1_dataout = (long *)0x4A31013C;
    long *gpio4_dataout = (long *)0x4805913C;

    bool toggle = false;
    while(true) {
        for (int i = 0; i < 1 << 29; ++i);
        if(toggle) {
            *gpio1_dataout = *gpio1_dataout & 0xFFFFFEFF;
            *gpio4_dataout = *gpio4_dataout & 0xFFFFBFFF;
        } else {
            *gpio1_dataout = *gpio1_dataout | 0x100;
            *gpio4_dataout = *gpio4_dataout | 0x4000;
        }
        toggle = !toggle;
    }
}

void
init_leds(void) {
    long *gpio1_oe = (long *)0x4A310134;
    long *gpio4_oe = (long *)0x48059134;

    *gpio1_oe = *gpio1_oe & 0xFFFFFEFF;
    *gpio4_oe = *gpio4_oe & 0xFFFFBFFF;

    long *mux = (long*)0x4A1000F4;
    *mux = *mux | 0x30000;
}
