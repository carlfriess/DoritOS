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
#include <aos/process.h>
#include <aos/lmp.h>
#include <spawn_serv.h>

#include <mm/mm.h>
#include "mem_alloc.h"
#include "multicore.h"
#include "m1_test.h"
#include "m2_test.h"


coreid_t my_core_id;
struct bootinfo *bi;
struct urpc_chan urpc_chan; // URPC channel for communicating with the other CPU

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
    
    
    // MARK: - Set up URPC (on APP)
    if (my_core_id != 0) {
        
        // URPC frame capability
        struct capref urpc_frame_cap = {
            .cnode = {
                .croot = CPTR_ROOTCN,
                .cnode = CPTR_TASKCN_BASE,
                .level = CNODE_TYPE_OTHER
            },
            .slot = TASKCN_SLOT_MON_URPC
        };
        
        err = frame_identify(urpc_frame_cap, &urpc_chan.fi);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "initialize urpc (1)");
        }
        
        err = paging_map_frame(get_current_paging_state(), (void **) &urpc_chan.buf, URPC_BUF_SIZE, urpc_frame_cap, NULL, NULL);
        if(err_is_fail(err)){
            DEBUG_ERR(err, "initialize urpc (2)");
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
        bi_frame_identities = (struct urpc_bi_caps *) urpc_recv_from_bsp(&urpc_chan);
        
        // Make sure we recieved it
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
        
        // We are done with the URPC message, so ack it.
        urpc_ack_recv_from_bsp(&urpc_chan);
        
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
    spawn_serv_init(&urpc_chan);

    
    // MARK: - Multicore
    
    // If on the BSP, boot the other core
    if (my_core_id == 0) {
        err = boot_core(1, &urpc_chan);
        if (err_is_fail(err)) {
            debug_printf("Failed booting core: %s\n", err_getstring(err));
        }
    }

    
    // MARK: - Message handling
    
    debug_printf("Message handler loop\n");
    // Hang around
    struct waitset *default_ws = get_default_waitset();
    while (true) {

        // Dispatch pending LMP events
        err = event_dispatch_non_block(default_ws);
        if (err_is_fail(err) && err != LIB_ERR_NO_EVENT) {
            DEBUG_ERR(err, "in event_dispatch");
            abort();
        }
        
        // Check if message received form different core
        void *recv_buf = urpc_recv(&urpc_chan);
        if (recv_buf != NULL) {

            if (*(enum urpc_msg_type *) recv_buf == URPC_MessageType_Spawn) {
                
                domainid_t pid;
                
                // Pass message to spawn server
                errval_t ret_err = spawn_serv_handler((char *) (recv_buf + sizeof(enum urpc_msg_type)), my_core_id, &pid);
        
                // Acknowledge urpc message
                urpc_ack_recv(&urpc_chan);
        
                // Composing message
                void *send_buf = urpc_get_send_buf(&urpc_chan);
                *(enum urpc_msg_type *) send_buf = URPC_MessageType_SpawnAck;
                *(errval_t *) (send_buf + sizeof(enum urpc_msg_type)) = ret_err;
                *(domainid_t *) (send_buf + sizeof(enum urpc_msg_type) + sizeof(errval_t)) = pid;
                
                // Send message back to requesting core
                urpc_send(&urpc_chan);
                
            }
            else {
                USER_PANIC("Unknown message type\n");
            }
        }
        
    }

    return EXIT_SUCCESS;
}
