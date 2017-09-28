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

#define DEFAULT_THRESHHOLD 400000


// Line status register (5th bit: TX_FIFO_E, 0th bit: RX_FIFO_E)
volatile char *UART3_LSR = (char *) 0x48020014;
    
// Transmit holding register
volatile char *UART3_THR = (char *) 0x48020000;
    
// Receive holding register
volatile char *UART3_RHR = (char *) 0x48020000;
    

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
    /* XXX - You'll need to implement this, but it's safe to ignore the
     * port parameter. */
    
    while(!((*UART3_LSR) & 0x00000020));
    *UART3_THR = c;
}

char
serial_getchar(unsigned port) {
    /* XXX - You only need to implement this if you're going for the extension
     * component. */

    while (!((*UART3_LSR) & 0x00000001));
    return *UART3_RHR;
    
}

/*** LED flashing ***/

__attribute__((noreturn))
void
blink_leds(void) {
    /* XXX - You'll need to implement this. */
    
    // LED D2 with signal name gpio_wk8
    volatile uint32_t *GPIO1_OE = (uint32_t *) 0x4A310134;
    volatile uint32_t *GPIO1_DATAOUT = (uint32_t *) 0x4A31013C;
    
    
    // LED D1 with signal name gpio_110 and Button S2 with gpio_113
    volatile uint32_t *GPIO4_OE = (uint32_t *) 0x48059134;
    volatile uint32_t *GPIO4_DATAOUT = (uint32_t *) 0x4805913C;
    volatile uint32_t *GPIO4_DATAIN = (uint32_t *) 0x48059138;
    
    
    // Multiplex [18:16] ABE_MCBSP2_CLKX_MUXMODE with 0x3 for gpio_110
    volatile uint32_t *CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX = (uint32_t *) 0x4A1000F4; 
        
    // Multiplex gpio_110
    *CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX |= 0x00030000;
    *CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX &= 0xFFFBFFFF;

    
    // Multiplex [2:0] ABE_MCBSP2_FSX_MUXMODE with 0x3 for gpio_113
    volatile uint32_t *CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX = (uint32_t *) 0x4A1000FC;
    
    // Multiplex gpio_113 and enable pullup
    *CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX |= 0x0000001B;
    *CONTROL_CORE_PAD0_ABE_MCBSP2_FSX_PAD1_ABE_MCBSP1_CLKX &= 0xFFFFFFFB;
    
    
    // Initialize Button S2
    
    printf("***********************************************\n");
    printf("* Press button S2 on the PandaBoard to start! *\n");
    printf("***********************************************\n");

    while((*GPIO4_DATAIN) & 0x00020000);
        
    // Initialize LEDs D1 and D2
    
    // Enable bit by setting 
    *GPIO1_OE &= 0xFFFFFEFF; //8th bit
    *GPIO4_OE &= 0xFFFFBFFF; //14th bit
    
    // Swith D2 on such that it is blinking alternating
    *GPIO4_DATAOUT ^= 0x00004000;

    // Mode: 0 BLINKING, 1 TOGGLE
    int mode = 0;
    
    // Counter for blinking
    int count = 0;
    
    // Threshhold for blinking
    int threshhold = (10-1) * DEFAULT_THRESHHOLD;
    
    // Input char
    char c;
    
    printf("*******************************************************\n");
    printf("* Change between modes BLINKING and TOGGLE with key 0 *\n");
    printf("* Control speed of blinking with keys 1 to 9          *\n");
    printf("* Toggle LEDs with keys 1 and 2                       *\n");
    printf("*******************************************************\n");
    
    while(1){
        
        switch(mode){
            case 0: /* BLINKING MODE */

                while(!((*UART3_LSR) & 0x1)){
                    if (count > threshhold){
                        *GPIO1_DATAOUT ^= 0x00000100;
                        *GPIO4_DATAOUT ^= 0x00004000;
                        count = 0;
                    }else{
                        count++;
                    }
                }
                
                c = *UART3_RHR;
                
                if ('0'< c && c <= '9'){
                    //printf("Changed speed to: %c\n", c);
                    int factor = c - '0';
                    threshhold = (10 - factor)*DEFAULT_THRESHHOLD;
                }else if (c == '0'){
                    printf("Changed mode to: TOGGLE\n");
                    mode = 1;
                }
                
                break;
                
            case 1: /* TOGGLE MODE */
                
                c = serial_getchar(0);
                
                if (c == '1'){
                    //printf("Toggled LED D1\n");
                    *GPIO1_DATAOUT ^= 0x00000100;
                }else if (c == '2'){
                    //printf("Toggled LED D2\n");
                    *GPIO4_DATAOUT ^= 0x00004000;
                }else if (c == '0'){
                    printf("Changed mode to: BLINKING\n");
                    mode = 0;
                }
                
                break;
                
        }
            
    }
    
}
