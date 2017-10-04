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

#include <mm/mm.h>
#include "mem_alloc.h"

coreid_t my_core_id;
struct bootinfo *bi;

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
    
    /* TESTS */
    printf("Test0\n");
    struct capref retcap0;
    ram_alloc(&retcap0, 64);
    aos_ram_free(retcap0, 64);
    
    printf("Test1\n");
    struct capref retcap1;
    ram_alloc(&retcap1, 64);
    aos_ram_free(retcap1, 64);
    
    printf("Test2\n");
    struct capref retcap2;
    ram_alloc(&retcap2, 4096);
    aos_ram_free(retcap2, 4096);
    
    //printf("Test3\n");
    //struct capref retcap3;
    //ram_alloc(&retcap3, 0);
    //aos_ram_free(retcap3, 0);
    
    printf("Test4\n");
    struct capref retcap4;
    struct capref retcap5;
    struct capref retcap6;

    ram_alloc(&retcap4, 2048);
    ram_alloc(&retcap5, 4096);
    ram_alloc(&retcap6, 4096);

    aos_ram_free(retcap6, 4096);
    aos_ram_free(retcap4, 2048);
    aos_ram_free(retcap5, 4096);
    


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
