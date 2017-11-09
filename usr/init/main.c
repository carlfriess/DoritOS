/**
 * \file
 * \brief init process for child spawning
 */

/*
 * Copyright (c) 2007, 2008, 2009, 2010, 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <barrelfish_kpi/types.h>
#include <target/arm/barrelfish_kpi/arm_core_data.h>
#include <aos/kernel_cap_invocations.h>
#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/morecore.h>
#include <aos/paging.h>
#include <aos/coreboot.h>
#include <spawn/spawn.h>
#include <spawn/multiboot.h>
#include <aos/process.h>
#include <aos/capabilities.h>
#include <aos/lmp.h>
#include <spawn_serv.h>
#include <elf/elf.h>

#include <mm/mm.h>
#include "mem_alloc.h"
#include "m1_test.h"
#include "m2_test.h"


coreid_t my_core_id;
struct bootinfo *bi;

static errval_t boot_core(coreid_t core_id, void **urpc_frame) {

    errval_t err;

    struct paging_state *st = get_current_paging_state();

    // Allocate frame for relocatable segment of kernel
    struct capref segment_frame_cap;
    size_t segment_size = 1100 * BASE_PAGE_SIZE;
    err = frame_alloc(&segment_frame_cap, segment_size, &segment_size);
    if (err_is_fail(err)) {
        return err;
    }

    // Map frame for relocatable segment of kernel
    void *segment_vaddr = NULL;
    err = paging_map_frame(st, &segment_vaddr, segment_size, segment_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Allocate frame for arm_core_data
    struct capref core_data_frame_cap;
    size_t core_data_size = sizeof(struct arm_core_data);
    err = frame_alloc(&core_data_frame_cap, core_data_size, &core_data_size);
    if (err_is_fail(err)) {
        return err;
    }

    // Map frame for arm_core_data
    struct arm_core_data *core_data = NULL;
    err = paging_map_frame(st, (void **) (&core_data), core_data_size, core_data_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Allocate frame for init process
    struct capref init_frame_cap;
    size_t init_size = ARM_CORE_DATA_PAGES * BASE_PAGE_SIZE;
    err = frame_alloc(&init_frame_cap, init_size, &init_size);
    if (err_is_fail(err)) {
        return err;
    }

    // Map frame for init process
//    void *init_vaddr = NULL;
//    err = paging_map_frame(st, &init_vaddr, init_size, init_frame_cap, NULL, NULL);
//    if (err_is_fail(err)) {
//        return err;
//    }

    // Allocate frame for URPC
    struct capref urpc_frame_cap;
    size_t urpc_size = MON_URPC_SIZE;
    err = frame_alloc(&urpc_frame_cap, urpc_size, &urpc_size);
    if (err_is_fail(err)) {
        return err;
    }

    // Map frame for URPC
    void *urpc_vaddr = NULL;
    err = paging_map_frame(st, urpc_frame, urpc_size, urpc_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    debug_printf("Set up KCB\n");

    // Allocate RAM for KCB
    struct capref kcb_ram_cap;
    size_t kcb_size = OBJSIZE_KCB;
    err = ram_alloc(&kcb_ram_cap, kcb_size);
    if (err_is_fail(err)) {
        return err;
    }

    debug_printf("Retype KCB capability\n");

    // Retype the RAM cap into a KCB cap
    struct capref kcb_cap;
    err = slot_alloc(&kcb_cap);
    if (err_is_fail(err)) {
        return err;
    }
    err = cap_retype(kcb_cap, kcb_ram_cap, 0, ObjType_KernelControlBlock, 0, 1);
    if (err_is_fail(err)) {
        return err;
    }

    debug_printf("Clone KCB\n");

    // Clone the current KCB and core data
    err = invoke_kcb_clone(kcb_cap, core_data_frame_cap);
    if (err_is_fail(err)) {
        return err;
    }

    // Find the cpu_omap44xx module
    struct mem_region *mem = multiboot_find_module(bi, "cpu_omap44xx");
    if (!mem) {
        return SPAWN_ERR_FIND_MODULE;
    }

    // Find the init module
    struct mem_region *init_mem = multiboot_find_module(bi, "init");
    if (!init_mem) {
        return SPAWN_ERR_FIND_MODULE;
    }

    // Assign init module as monitor module in core data
    core_data->monitor_module.mod_start = init_mem->mr_base;
    core_data->monitor_module.mod_end = init_mem->mr_base + init_mem->mrmod_size;
    core_data->monitor_module.string = init_mem->mrmod_data;
    core_data->monitor_module.reserved = init_mem->mrmod_slot;

    // Constructing the capability for the frame containing the ELF image
    struct capref elf_frame = {
        .cnode = cnode_module,
        .slot = mem->mrmod_slot
    };

    void *elf_buf = NULL;
    err = paging_map_frame_attr(st, &elf_buf, mem->mrmod_size, elf_frame, VREGION_FLAGS_READ, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Get frame identities
    struct frame_identity core_data_identity;
    err = frame_identify(core_data_frame_cap, &core_data_identity);
    if (err_is_fail(err)) {
        return err;
    }

    struct frame_identity segment_frame_identity;
    err = frame_identify(segment_frame_cap, &segment_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }

    struct frame_identity init_frame_identity;
    err = frame_identify(init_frame_cap, &init_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }

    struct frame_identity urpc_frame_identity;
    err = frame_identify(urpc_frame_cap, &urpc_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }

    struct frame_identity kcb_frame_identity;
    err = frame_identify(kcb_cap, &kcb_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }

    // Set the location of the KCB
    core_data->kcb = kcb_frame_identity.base;

    // Set memory region for loading init
    core_data->memory_base_start = init_frame_identity.base;
    core_data->memory_bytes = init_frame_identity.bytes;

    // Set the location of the URPC frame
    core_data->urpc_frame_base = (uint32_t) urpc_frame_identity.base;
    core_data->urpc_frame_size = (uint32_t) urpc_frame_identity.bytes;

    // Set the name of the init process
    strcpy(core_data->init_name, "init");
    
    // Set the commandline for the new kernel
    core_data->cmdline = init_mem->mr_base + init_mem->mrmod_data;

    err = load_cpu_relocatable_segment(elf_buf, segment_vaddr, segment_frame_identity.base, core_data->kernel_load_base, &core_data->got_base);
    if (err_is_fail(err)) {
        return err;
    }

    // Clean and invalidate cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) segment_frame_identity.base), (void *) ((uint32_t) segment_frame_identity.base + (uint32_t) segment_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) segment_frame_identity.base), (void *) ((uint32_t) segment_frame_identity.base + (uint32_t) segment_frame_identity.bytes));
    sys_armv7_cache_clean_poc((void *) ((uint32_t) init_frame_identity.base), (void *) ((uint32_t) init_frame_identity.base + (uint32_t) init_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) init_frame_identity.base), (void *) ((uint32_t) init_frame_identity.base + (uint32_t) init_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) urpc_frame_identity.base), (void *) ((uint32_t) urpc_frame_identity.base + (uint32_t) urpc_frame_identity.bytes));

    debug_printf("Booting Core\n");
    
    // Boot the code
    invoke_monitor_spawn_core(1, CPU_ARM7, core_data_identity.base);

    debug_printf("BOOTED\n");
    return err;
}

int main(int argc, char *argv[])
{
    errval_t err;

    /* Set the core id in the disp_priv struct */
    err = invoke_kernel_get_core_id(cap_kernel, &my_core_id);
    assert(err_is_ok(err));
    disp_set_core_id(my_core_id);

    debug_printf("init: on core %" PRIuCOREID " invoked as:", my_core_id);
    for (int i = 0; i < argc; i++) {
       printf(" %s", argv[i]);
    }
    printf("\n");

    /* First argument contains the bootinfo location, if it's not set */
    bi = (struct bootinfo*)strtol(argv[1], NULL, 10);
    if (!bi) {
        assert(my_core_id > 0);
    }

    err = initialize_ram_alloc();
    if(err_is_fail(err)){
        DEBUG_ERR(err, "initialize_ram_alloc");
    }

    // Retype init's dispatcher capability to create an endpoint
    err = cap_retype(cap_selfep, cap_dispatcher, 0, ObjType_EndPoint, 0, 1);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Setting aos_ram_free function pointer to ram_free_handler in lmp.c
    register_ram_free_handler(aos_ram_free);
    
    // Run tests:

    // Milestone 1:
    //run_all_m1_tests();

    // Milestone 2:
    //run_all_m2_tests();

    // Initialize the spawn server
    spawn_serv_init();

    if (!my_core_id) {

        err = boot_core(1);
        if (err_is_fail(err)) {
            debug_printf("Failed booting core: %s\n", err_getstring(err));
        }

    }

    // Allocate spawninfo
//    struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));

    // Spawn memeater
//    spawn_load_by_name("memeater", si);

    // Free the process info for memeater
//    free(si);

    debug_printf("Message handler loop\n");
    // Hang around
    struct waitset *default_ws = get_default_waitset();
    while (true) {

        err = event_dispatch(default_ws);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "in event_dispatch");
            abort();
        }
    }

    return EXIT_SUCCESS;
}
