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

int test_alloc_free(size_t);

int test_alloc_free_alloc_free(size_t, size_t);

int test_alloc_free_free(size_t);

int test_alloc_n_free_n(int, size_t);
    
int test_coalescing1(size_t, size_t, size_t);


coreid_t my_core_id;
struct bootinfo *bi;

int test_alloc_free(size_t b) {
    printf("a1 f1 with (%d)\n", b);
    struct capref retcap;
    
    ram_alloc(&retcap, b);
    aos_ram_free(retcap, b);
    
    return 0;
}


int test_alloc_free_alloc_free(size_t b1, size_t b2) {
    printf("a1 f1 a2 f2 with (%d,%d)\n", b1, b2);
    struct capref retcap1;
    struct capref retcap2;
    
    ram_alloc(&retcap1, b1);
    aos_ram_free(retcap1, b1);
    ram_alloc(&retcap2, b2);
    aos_ram_free(retcap2, b2);
    
    return 0;
}

int test_alloc_free_free(size_t b) {
    printf("a1 f1 f1 with (%d)\n", b);
    struct capref retcap;
    
    ram_alloc(&retcap, b);
    aos_ram_free(retcap, b);
    aos_ram_free(retcap, b);

    return 0;
}

int test_alloc_n_free_n(int n, size_t b) {
    printf("a1..an and f1..fn with (%d)\n");
    struct capref retcap_array[n];
    
    for (int i=0; i<n; ++i) {
        printf("Allocating mmnode: %d\n", i+1);
        ram_alloc(&(retcap_array[i]), b);
    }
    for (int i=0; i<n; ++i) {
        aos_ram_free(retcap_array[i], b);
    }
    
    return 0;
}

int test_coalescing1(size_t b1, size_t b2, size_t b3) {
    printf("a1 a2 a3 f3 f1 f2 with (%d,%d,%d)\n", b1, b2, b3);
    struct capref retcap_1;
    struct capref retcap_2;
    struct capref retcap_3;
    
    ram_alloc(&retcap_1, b1);
    ram_alloc(&retcap_2, b2);
    ram_alloc(&retcap_3, b3);
    
    aos_ram_free(retcap_3, b3);
    aos_ram_free(retcap_1, b1);
    aos_ram_free(retcap_2, b2);
    
    return 0;
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
    
    /* TESTS */
    /*printf("Test 1\n");
    assert(!test_alloc_free_alloc_free(64, 64));
    
    printf("Test 2\n");
    assert(!test_alloc_free(4096));
    //assert(!test_alloc_free(0));      //< Should this be fixed?
    
    printf("Test 3\n");
    assert(!test_alloc_free_free(2*4096));
    */
    printf("Test 4\n");
    assert(!test_alloc_n_free_n(64, 2048));
    //assert(!test_alloc_n_free_n(65, 4096));

    //assert(!test_alloc_n_free_n(500, 4096));
    //assert(!test_alloc_n_free_n(500, 3*4096));
    
    //printf("Test 5\n");
    //assert(!test_coalescing1(2048, 4096, 2*4096));
    //assert(!test_coalescing1(2*4096, 4096, 1));
    /*
    //printf("Test 6\n");
    // TODO: Check other coalescing cases*/
    
    /*struct capref frame;
    size_t retSize;
    errval_t err1 = frame_alloc(&frame, 4096, &retSize);
    debug_printf("Allocated a %zu byte frame: %s\n", retSize, err_getstring(err1));
    
    paging_map_fixed_attr(NULL, 0x0320A000, frame, 4096, VREGION_FLAGS_READ_WRITE);
    
    int *test = (int *) 0x0320A000;
    debug_printf("%d", *test);*/
    
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
