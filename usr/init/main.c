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
#include <spawn/spawn.h>
#include <aos/process.h>
#include <aos/capabilities.h>
#include <aos/lmp.h>
#include <spawn_serv.h>

#include <mm/mm.h>
#include "mem_alloc.h"
#include "m1_test.h"
#include "m2_test.h"


coreid_t my_core_id;
struct bootinfo *bi;

static errval_t boot_core(coreid_t core_id) {

    errval_t err;

    struct paging_state *st = get_current_paging_state();

    struct arm_core_data *core_data = malloc(sizeof(struct arm_core_data));
    if (core_data == NULL)  {
        return LIB_ERR_MALLOC_FAIL;
    }

    struct capref process_frame_cap;
    size_t size = ARM_CORE_DATA_PAGES * BASE_PAGE_SIZE;
    err = frame_alloc(&process_frame_cap, size, &size);
    if (err_is_fail(err)) {
        return err;
    }
    core_data->memory_bytes = (uint32_t) size;

    void *process_vaddr = NULL;
    err = paging_map_frame(st, &process_vaddr, size, process_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    core_data->memory_base_start = (uint32_t) process_vaddr;

    struct capref urpc_frame_cap;
    size = MON_URPC_SIZE;
    err = frame_alloc(&urpc_frame_cap, size, &size);
    if (err_is_fail(err)) {
        return err;
    }
    core_data->urpc_frame_size = (uint32_t) size;

    void *urpc_vaddr = NULL;
    err = paging_map_frame(st, &urpc_vaddr, size, urpc_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    core_data->urpc_frame_base = (uint32_t) urpc_vaddr;

    struct capref kcb_frame_cap;
    size = OBJSIZE_KCB;
    err = frame_alloc(&kcb_frame_cap, size, &size);
    if (err_is_fail(err)) {
        return err;
    }

    void *kcb_vaddr = NULL;
    err = paging_map_frame(st, &kcb_vaddr, size, kcb_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    core_data->kcb = (lvaddr_t) kcb_vaddr;

    struct capref current_kcb_cap = {
       .cnode = cnode_root,
       .slot = ROOTCN_SLOT_BSPKCB
    };
    err = invoke_kcb_clone(current_kcb_cap, kcb_frame_cap);
    if (err_is_fail(err)) {
        return err;
    }

    core_data->src_core_id = my_core_id;

    debug_printf("ADDR: %p\n", core_data->entry_point);

    struct mem_region *mem = multiboot_find_module(bi, "omap44xx");
    if (!mem) {
        return SPAWN_ERR_FIND_MODULE;
    }

    // Constructing the capability for the frame containing the ELF image
    struct capref elf_frame = {
        .cnode = cnode_module,
        .slot = mem->mrmod_slot
    };

    void *elf_buf = NULL
    err = paging_map_frame(st, &elf_buf, mem->mrmod_size, elf_frame, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    err = load_cpu_relocatable_segment(elf_buf, process_vaddr, lvaddr_t vbase, lvaddr_t text_base, lvaddr_t *got_base);
    if (err_is_fail(err)) {
        return err;
    }

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
