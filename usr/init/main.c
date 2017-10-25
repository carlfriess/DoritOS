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

#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/morecore.h>
#include <aos/paging.h>
#include <spawn/spawn.h>

#include <mm/mm.h>
#include "mem_alloc.h"
#include "m1_test.h"
#include "m2_test.h"


coreid_t my_core_id;
struct bootinfo *bi;

void simple_handler(void *arg);

void simple_handler(void *arg) {
    debug_printf("\n\n\nOH SHIT!\n\n\n");
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

    err = cap_retype(cap_selfep, cap_dispatcher, 0, ObjType_EndPoint, 0, 1);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    // Run tests:

    // Milestone 1:
    //run_all_m1_tests();

    // Milestone 2:
    //run_all_m2_tests();

    // Allocate and initialize lmp channel
    struct lmp_chan *lmp_channel = (struct lmp_chan *) malloc(sizeof(struct lmp_chan));

    // Open channel to messages
    err = lmp_chan_accept(lmp_channel, LMP_RECV_LENGTH, NULL_CAP);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));
    spawn_load_by_name("memeater", si);
    free(si);

    debug_printf("Message handler loop\n");
    // Hang around
    struct waitset *default_ws = get_default_waitset();

    // Register callback handler
    err = lmp_chan_register_recv(lmp_channel, default_ws, MKCLOSURE(simple_handler, (void *) lmp_channel));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    err = lmp_chan_alloc_recv_slot(lmp_channel);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    while (true) {

//        print_process_list();

        err = event_dispatch(default_ws);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "in event_dispatch");
            abort();
        }
    }

    return EXIT_SUCCESS;
}
