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

// Define register addresses for UART:
#define UART_THR    ((volatile char*)0x48020000)
#define UART_RHR    ((volatile char*)0x48020000)
#define UART_LSR    ((volatile char*)0x48020014)

// Define register addresses for GPIO:
#define GPIO1_OE        ((volatile unsigned int*)0x4A310134)
#define GPIO1_DATAOUT   ((volatile unsigned int*)0x4A31013C)
#define GPIO4_OE        ((volatile unsigned int*)0x48059134)
#define GPIO4_DATAIN    ((volatile unsigned int*)0x48059138)
#define GPIO4_DATAOUT   ((volatile unsigned int*)0x4805913C)
#define CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX  ((volatile unsigned int*)0x4A1000F4)
#define CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX  ((volatile unsigned int*)0x4A1000FC)

void blink_leds(void);

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
    // Wait for the TX FIFO to be empty:
    while (!((*UART_LSR) & 0x20)) ;
    // Write the next character to the TX holding register:
    *UART_THR = c;
}

char
serial_getchar(unsigned port) {
    // Wait to receive a character:
    while (!((*UART_LSR) & 0x01)) ;
    // Read the receiver character:
    return *UART_RHR;
}

/*** LED flashing ***/

__attribute__((noreturn))
void
blink_leds(void) {
    
    // Configure pins as Outputs:
    *GPIO1_OE &= 0xFFFFFEFF;    // GPIO_WK8
    *GPIO4_OE &= 0xFFFFBFFF;    // GPIO_110
    
    // Configure multiplexter for GPIO_110
    *CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX &= 0xFFFBFFFF;
    *CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX |= 0x00030000;
    
    // Configure multiplexter for GPIO_113 and enable internal pull-up
    *CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX &= 0xFFFFFFFB;
    *CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX |= 0x0000001B;
    
    printf("Waiting for you to press S2! ;)\n");
    
    while (*GPIO4_DATAIN & 0x00020000) ;    // Read GPIO_113
    
    unsigned int ledOn = 0;
    
blink:
    printf("Blinking LEDs...\n");
    printf("Type 1, 2 or 3 to override.\n");
    while (1) {
        ledOn = !ledOn;
        if (ledOn) {
            *GPIO1_DATAOUT |= 0x00000100;   // Set GPIO_WK8
            *GPIO4_DATAOUT &= 0xFFFFBFFF;   // Clear GPIO_110
        }
        else {
            *GPIO1_DATAOUT &= 0xFFFFFEFF;   // Clear GPIO_WK8
            *GPIO4_DATAOUT |= 0x00004000;   // Set GPIO_110
        }
        // Wait a bit..
        for (unsigned int i = 0; i < 1<<21; i++) {
            // Check if a character has been received:
            if ((*UART_LSR) & 0x01) {
                goto manual;
            }
        }
    };
    
manual:
    printf("Manual Control!\n");
    printf("Use space to return to automatic.\n");
    while (1) {
        switch (serial_getchar(0)) {
            case '1':
                *GPIO1_DATAOUT &= 0xFFFFFEFF;   // Clear GPIO_WK8
                *GPIO4_DATAOUT |= 0x00004000;   // Set GPIO_110
                break;
                
            case '2':
                *GPIO1_DATAOUT |= 0x00000100;   // Set GPIO_WK8
                *GPIO4_DATAOUT &= 0xFFFFBFFF;   // Clear GPIO_110
                break;
                
            case '3':
                *GPIO1_DATAOUT |= 0x00000100;   // Set GPIO_WK8
                *GPIO4_DATAOUT |= 0x00004000;   // Set GPIO_110
                break;
                
            case ' ':
                goto blink;
                break;
                
            default:
                *GPIO1_DATAOUT &= 0xFFFFFEFF;   // Clear GPIO_WK8
                *GPIO4_DATAOUT &= 0xFFFFBFFF;   // Clear GPIO_110
                break;
        }
    }
    
}
