/**
 * \file
 * \brief Implementation of AOS rpc-like messaging
 */

/*
 * Copyright (c) 2013 - 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos_rpc.h>
#include <aos/lmp.h>

size_t aos_rpc_terminal_write(const char* buf, size_t len) {
    errval_t err = SYS_ERR_OK;

    struct aos_rpc *chan = aos_rpc_get_serial_channel();

    for (size_t i = 0; i < len; i++) {
        err = aos_rpc_serial_putchar(chan, buf[i]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }
    }

    return 0;
}

size_t aos_rpc_terminal_read(char *buf, size_t len) {
    errval_t err = SYS_ERR_OK;

    struct aos_rpc *chan = aos_rpc_get_serial_channel();

    for (size_t i = 0; i < len; i++) {
        err = aos_rpc_serial_getchar(chan, &buf[i]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }
    }

    return 0;
}

errval_t aos_rpc_send_number(struct aos_rpc *chan, uintptr_t val)
{
    // TODO: implement functionality to send a number over the channel
    // given channel and wait until the ack gets returned.
    
    errval_t err;
    
    // Send the number over the channel
    err = lmp_chan_send2(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_Number,
                         val);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    // Wait to receive an acknowledgement
    lmp_client_recv(chan->lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == LMP_RequestType_Number);
    
    // Check we received the same value as we sent
    return msg.words[1] == val ? SYS_ERR_OK : -1;
}

errval_t aos_rpc_send_string(struct aos_rpc *chan, const char *string)
{
    
    return lmp_send_string(chan->lc, string);
    
}

errval_t aos_rpc_get_ram_cap(struct aos_rpc *chan, size_t size, size_t align,
                             struct capref *retcap, size_t *ret_size)
{
    // TODO: implement functionality to request a RAM capability over the
    // given channel and wait until it is delivered.
    
    errval_t err = SYS_ERR_OK;
    
    // Make sure that there are enough slots in advance.
    // If there are not, this will trigger a refill.
    struct capref dummy_slot;
    slot_alloc(&dummy_slot);
    slot_free(dummy_slot);
    
    err = lmp_chan_send3(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryAlloc, size, align);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Initializing message
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    do {
    
        // Receive the response
        lmp_client_recv(chan->lc, retcap, &msg);
        
        // Check if we got the message we wanted
        if (msg.words[0] != LMP_RequestType_MemoryAlloc) {
            
            debug_printf("Got ack of type: %d\n", msg.words[0]);
            
            // Request resend
            err = lmp_chan_send9(chan->lc, LMP_SEND_FLAGS_DEFAULT, *retcap, LMP_RequestType_Echo, msg.words[0], msg.words[1], msg.words[2], msg.words[3], msg.words[4], msg.words[5], msg.words[6], msg.words[7]);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
                return err;
            }
            
            // Allocate a new slot if necessary
            if (!capref_is_null(*retcap)) {
                err = lmp_chan_alloc_recv_slot(chan->lc);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                    return err;
                }
            }
            
        }
    
    } while (msg.words[0] != LMP_RequestType_MemoryAlloc);
    
    // Allocate recv slot
    err = lmp_chan_alloc_recv_slot(chan->lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // TODO: Implement ret_size
    *ret_size = size;
    
    // Set err to error of response message
    err = msg.words[1];
    
    return err;
}

errval_t aos_rpc_serial_getchar(struct aos_rpc *chan, char *retc)
{
    errval_t err = SYS_ERR_OK;

    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    err = lmp_chan_send1(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_TerminalGetChar);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Wait for receive
    lmp_client_recv(chan->lc, &cap, &msg);

    *retc = msg.words[2];

    return msg.words[1];
}


errval_t aos_rpc_serial_putchar(struct aos_rpc *chan, char c)
{
    errval_t err = SYS_ERR_OK;

    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    err = lmp_chan_send2(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_TerminalPutChar, (size_t) c);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Wait for receive
    lmp_client_recv(chan->lc, &cap, &msg);

    return msg.words[1];
}

errval_t aos_rpc_process_spawn(struct aos_rpc *chan, char *name,
                               coreid_t core, domainid_t *newpid)
{
    
    errval_t err;
    
    // Send spawn request with send_short_buf() or send_frame() depending on size of name
    err = lmp_send_spawn(chan->lc, name, core);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the status code and pid form the spawn server
    lmp_client_recv(chan->lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == LMP_RequestType_SpawnShort ||
           msg.words[0] == LMP_RequestType_SpawnLong);
    
    // Return the PID of the new process
    *newpid = msg.words[2];
    
    // Return the status code
    err = (errval_t) msg.words[1];
    return err;
    
}

errval_t aos_rpc_process_get_name(struct aos_rpc *chan, domainid_t pid,
                                  char **name)
{
    // TODO (milestone 5): implement name lookup for process given a process id
    
    errval_t err;
    
    err = lmp_chan_send2(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_NameLookup,
                         pid);
    if (err_is_fail(err)) {
        return err;
    }
    
    return lmp_recv_string(chan->lc, name);
}

errval_t aos_rpc_process_get_all_pids(struct aos_rpc *chan,
                                      domainid_t **pids, size_t *pid_count)
{
    // TODO (milestone 5): implement process id discovery
    
    errval_t err;
    
    err = lmp_chan_send1(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_PidDiscover);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the number of PIDs form the spawn server
    lmp_client_recv(chan->lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == LMP_RequestType_PidDiscover);
    
    // Return the PID count
    *pid_count = msg.words[1];

    //  TODO: Refactor receiving of PID array!

    // Receive the array of PIDs
    struct capref array_frame;
    size_t array_frame_size;
    err = lmp_recv_frame(chan->lc, LMP_RequestType_StringLong, &array_frame, &array_frame_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Map the array into memory
    err = paging_map_frame(get_current_paging_state(), (void **) pids,
                           array_frame_size, array_frame, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    return err;
}

errval_t aos_rpc_get_device_cap(struct aos_rpc *chan,
                                lpaddr_t paddr, size_t bytes,
                                struct capref *frame)
{
    
    errval_t err;
    
    // Send request to get device capability in [paddr, paddr + bytes]
    err = lmp_chan_send3(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_DeviceCap,
                         paddr,
                         bytes);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Initialize message
    //struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the devframe capability
    lmp_client_recv(chan->lc, frame, &msg);
    
    struct frame_identity id;
    invoke_frame_identify(*frame, &id);
    
    debug_printf("Received %llx %llu\n", id.base, id.bytes);
    
    //debug_printf("%s\n", err_getstring((errval_t) msg.words[1]));
    
    // Allocate recv slot
    err = lmp_chan_alloc_recv_slot(chan->lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    return err;
    
}

errval_t aos_rpc_init(struct aos_rpc *rpc, struct lmp_chan *lc)
{
    // TODO: Initialize given rpc channel
    rpc->lc = lc;
    return SYS_ERR_OK;
}

/**
 * \brief Returns the RPC channel to init.
 */
struct aos_rpc *aos_rpc_get_init_channel(void)
{
    return get_init_rpc();
}

/**
 * \brief Returns the channel to the memory server
 */
struct aos_rpc *aos_rpc_get_memory_channel(void)
{
    return aos_rpc_get_init_channel();
}

/**
 * \brief Returns the channel to the process manager
 */
struct aos_rpc *aos_rpc_get_process_channel(void)
{
    return aos_rpc_get_init_channel();
}

/**
 * \brief Returns the channel to the serial console
 */
struct aos_rpc *aos_rpc_get_serial_channel(void)
{
    return aos_rpc_get_init_channel();
}
