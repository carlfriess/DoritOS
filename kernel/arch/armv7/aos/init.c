/*
 * Copyright (c) 2009-2016, ETH Zurich. All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.  If
 * you do not find this file, copies can be found by writing to: ETH Zurich
 * D-INFK, CAB F.78, Universitaetstr. 6, CH-8092 Zurich, Attn: Systems Group.
 */

/**
 * \file
 * \brief AOS Milestone 0 CPU driver
 */
#include <kernel.h>

#include <barrelfish_kpi/arm_core_data.h>
#include <bitmacros.h>
#include <coreboot.h>
#include <cp15.h>
#include <dev/cpuid_arm_dev.h>
#include <exceptions.h>
#include <getopt/getopt.h>
#include <gic.h>
#include <global.h>
#include <init.h>
#include <kcb.h>
#include <kernel_multiboot.h>
#include <offsets.h>
#include <paging_kernel_arch.h>
#include <platform.h>
#include <serial.h>
#include <startup_arch.h>
#include <stdio.h>
#include <string.h>

/*
 * Forward declarations
 */

#define MSG(format, ...) printk( LOG_NOTE, "ARMv7-A: "format, ## __VA_ARGS__ )

/* The BSP kernel allocates its coreboot data in its BSS, all other cores will
 * overwrite this pointer with that passed to them at boot time. */
struct arm_core_data *core_data;

/* There is only one copy of the global locks, which is allocated alongside
 * the BSP kernel.  All kernels have their pointers set to the BSP copy. */
struct global *global= NULL;

coreid_t my_core_id;

/* Implemented in kernel/arch/armv7/aos/plat_omap44xx.c */
void blink_leds(void);

//
// Kernel command line variables and binding options
//

uint32_t periphclk = 0;
uint32_t periphbase = 0;
uint32_t timerirq = 0;

static struct cmdarg cmdargs[] = {
    { "consolePort", ArgType_UInt, { .uinteger = (void *)0 } },
    { "loglevel",    ArgType_Int,  { .integer  = (void *)0 } },
    { "logmask",     ArgType_Int,  { .integer  = (void *)0 } },
    { NULL, 0, { NULL } }
};

static void
init_cmdargs(void) {
    cmdargs[0].var.uinteger= &serial_console_port;
    cmdargs[1].var.integer=  &kernel_loglevel;
    cmdargs[2].var.integer=  &kernel_log_subsystem_mask;
}

/**
 * \brief Is this the BSP?
 * \return True iff the current core is the bootstrap processor.
 */
bool cpu_is_bsp(void)
{
    return true;
}

/**
 * \brief Continue kernel initialization in kernel address space.
 *
 * This function sets up exception handling, initializes devices and enables
 * interrupts. After that it calls arm_kernel_startup(), which should not
 * return (if it does, this function halts the kernel).
 */
void
arch_init(struct arm_core_data *boot_core_data,
          struct armv7_boot_record *bootrec)
{
    /* Now we're definitely executing inside the kernel window, with
     * translation and caches available, and all pointers relocated to their
     * correct virtual address.  The low mappings are still enabled, but we
     * shouldn't be accessing them any longer, no matter where RAM is located.
     * */
    
    /* Save our core data. */
    core_data= boot_core_data;

    /* Let the paging code know where the kernel page tables are.  Note that
     * paging_map_device() won't work until this is called. */
    paging_load_pointers(core_data);

    /* Reinitialise the serial port, as it may have moved, and we need to map
     * it into high memory. */
    /* XXX - reread the args to update serial_console_port. */
    serial_console_init(true);
    
    
    /* Load the global lock address. */
    global= (struct global *)core_data->global;

    MSG("Barrelfish CPU driver starting on ARMv7\n");
    MSG("Core data at %p\n", core_data);
    MSG("Global data at %p\n", global);
    assert(core_data != NULL);
    assert(paging_mmu_enabled());

    my_core_id = cp15_get_cpu_id();
    MSG("Set my core id to %d\n", my_core_id);

    struct multiboot_info *mb=
        (struct multiboot_info *)core_data->multiboot_header;
    
    MSG("Multiboot info:\n");
    MSG(" info header at 0x%"PRIxLVADDR"\n",       (lvaddr_t)mb);
    MSG(" mods_addr is P:0x%"PRIxLPADDR"\n",       (lpaddr_t)mb->mods_addr);
    MSG(" mods_count is 0x%"PRIu32"\n",                      mb->mods_count);
    MSG(" cmdline is at P:0x%"PRIxLPADDR"\n",      (lpaddr_t)mb->cmdline);
    MSG(" cmdline reads '%s'\n", local_phys_to_mem((lpaddr_t)mb->cmdline));
    MSG(" mmap_length is 0x%"PRIu32"\n",                     mb->mmap_length);
    MSG(" mmap_addr is P:0x%"PRIxLPADDR"\n",       (lpaddr_t)mb->mmap_addr);
    MSG(" multiboot_flags is 0x%"PRIu32"\n",                 mb->flags);

    MSG("Parsing command line\n");
    init_cmdargs();
    parse_commandline((const char *)core_data->cmdline, cmdargs);

    MSG("Welcome to AOS.\n");
        
    serial_putchar(0,42);
    serial_putchar(0,42);
    serial_putchar(0,42);
    
    printf("\n");
    
    blink_leds();

    while(1);
}
