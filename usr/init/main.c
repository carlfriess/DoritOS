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
#include <aos/waitset_chan.h>
#include <aos/event_queue.h>
#include <aos/morecore.h>
#include <aos/paging.h>
#include <spawn/spawn.h>
#include <aos/process.h>
#include <aos/lmp.h>
#include <aos/urpc.h>
#include <spawn_serv.h>

#include <mm/mm.h>
#include "mem_alloc.h"
#include "multicore.h"
#include "m1_test.h"
#include "m2_test.h"


coreid_t my_core_id;
struct bootinfo *bi;
extern struct ump_chan init_uc; // UMP channel for communicating with the other CPU

struct event_queue_pair {
    struct event_queue queue;
    struct event_queue_node node;
};

static void ump_event_handler(void *arg) {
    // Reregister event node
    struct event_queue_pair *pair = arg;
    event_queue_add(&pair->queue, &pair->node, MKCLOSURE(ump_event_handler, (void *) pair));

    // Check if a message was received form different core
    errval_t err;
    void *msg;
    size_t msg_size;
    ump_msg_type_t msg_type;
    err = ump_recv(&init_uc, &msg, &msg_size, &msg_type);
    if (err_is_ok(err)) {

        // Invoke the URPC server
        urpc_init_server_handler(&init_uc, msg, msg_size, msg_type);
        
        // Free the received message buffer
        free(msg);
        
    }
    else if (err != LIB_ERR_NO_UMP_MSG) {
        DEBUG_ERR(err, "in urpc_recv");
    }
}

int main(int argc, char *argv[])
{
    errval_t err;
    struct urpc_bi_caps *bi_frame_identities = NULL;
    

    /* Set the core id in the disp_priv struct */
    err = invoke_kernel_get_core_id(cap_kernel, &my_core_id);
    assert(err_is_ok(err));
    disp_set_core_id(my_core_id);

    debug_printf("init: on core %" PRIuCOREID " invoked as:", my_core_id);
    for (int i = 0; i < argc; i++) {
       printf(" %s", argv[i]);
    }
    printf("\n");
    
    
    // MARK: - Set up UMP (on APP)
    if (my_core_id != 0) {
        
        // Initialize the UMP channel
        ump_chan_init(&init_uc, UMP_APP_BUF_SELECT);
        
        // UMP frame capability
        struct capref urpc_frame_cap = {
            .cnode = {
                .croot = CPTR_ROOTCN,
                .cnode = CPTR_TASKCN_BASE,
                .level = CNODE_TYPE_OTHER
            },
            .slot = TASKCN_SLOT_MON_URPC
        };
        
        err = frame_identify(urpc_frame_cap, &init_uc.fi);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "initialize ump (1)");
        }
        
        err = paging_map_frame(get_current_paging_state(), (void **) &init_uc.buf, UMP_BUF_SIZE, urpc_frame_cap, NULL, NULL);
        if(err_is_fail(err)){
            DEBUG_ERR(err, "initialize ump (2)");
        }
        
    }
    
    
    // MARK: - Get Bootinfo
    if (my_core_id == 0) {  // BSP
        
        /* First argument contains the bootinfo location, if it's not set */
        bi = (struct bootinfo*)strtol(argv[1], NULL, 10);
        if (!bi) {
            assert(my_core_id > 0);
        }
        
    }
    else {  // APP
       
        // Receive the bootinfo frame identity from the BSP
        size_t recv_size;
        ump_msg_type_t msg_type;
        err = ump_recv(&init_uc,
                       (void **) &bi_frame_identities,
                       &recv_size,
                       &msg_type);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "bootinfo receive");
        }
        
        // Make sure we recieved it
        assert(err_is_ok(err));
        assert(msg_type == UMP_MessageType_Bootinfo);
        assert(bi_frame_identities != NULL);
        
        // Forge bootinfo frame capability
        struct capref bi_frame_cap = {
            .cnode = {
                .croot = CPTR_ROOTCN,
                .cnode = CPTR_TASKCN_BASE,
                .level = CNODE_TYPE_OTHER
            },
            .slot = TASKCN_SLOT_BOOTINFO
        };
        err = frame_forge(bi_frame_cap,
                          bi_frame_identities->bootinfo.base,
                          bi_frame_identities->bootinfo.bytes,
                          my_core_id);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "bootinfo frame forge");
        }
        
        // Map the bootinfo frame
        err = paging_map_frame(get_current_paging_state(),
                               (void **) &bi,
                               bi_frame_identities->bootinfo.bytes,
                               bi_frame_cap,
                               NULL,
                               NULL);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "bootinfo frame map");
        }
        
    }
    
    
    // MARK: - RAM setup
    err = initialize_ram_alloc(my_core_id);
    if(err_is_fail(err)){
        DEBUG_ERR(err, "initialize_ram_alloc");
    }
    
    // MARK: - Forge module caps (on APP)
    if (my_core_id != 0) {
        
        err = forge_module_caps(bi_frame_identities, my_core_id);
        if(err_is_fail(err)){
            DEBUG_ERR(err, "forging module caps");
        }
        
        // We are done with the UMP message, so free it.
        free(bi_frame_identities);
        
    }
    
    
    // MARK: - LMP setup

    // Retype init's dispatcher capability to create an endpoint
    err = cap_retype(cap_selfep, cap_dispatcher, 0, ObjType_EndPoint, 0, 1);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Setting aos_ram_free function pointer to ram_free_handler in lmp.c
    register_ram_free_handler(aos_ram_free);

    // Initialize the spawn server
    spawn_serv_init(&init_uc);

    
    // MARK: - Multicore
    
    // If on the BSP, boot the other core
    if (my_core_id == 0) {
        err = boot_core(1, &init_uc);
        if (err_is_fail(err)) {
            debug_printf("Failed booting core: %s\n", err_getstring(err));
        }
    }

    
    // MARK: - Message handling
    
    debug_printf("Message handler loop\n");
    // Hang around
    struct waitset *default_ws = get_default_waitset();

    // Register Event Queue for UMP

    struct event_queue_pair pair;
    event_queue_init(&pair.queue, default_ws, EVENT_QUEUE_CONTINUOUS);
    event_queue_add(&pair.queue, &pair.node, MKCLOSURE(ump_event_handler, (void *) &pair));

    while (true) {
        event_dispatch(default_ws);
    }

    return EXIT_SUCCESS;
}
