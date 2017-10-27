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
    
    // Wait to receive an acknowledgement
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    lmp_client_recv(chan->lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == LMP_RequestType_Number);
    
    // Check we received the same value as we sent
    return msg.words[1] == val ? SYS_ERR_OK : -1;
}

errval_t aos_rpc_send_string(struct aos_rpc *chan, const char *string)
{
    errval_t err;
    
    // Get length of the string
    size_t len = strlen(string);
    
    // Check wether to use StringShort or StringLong protocol
    if (len < sizeof(uintptr_t) * 8) {
        
        // Allocate new memory to construct the arguments
        char *string_arg = calloc(sizeof(uintptr_t), 8);
        
        // Copy in the string
        memcpy(string_arg, string, len);
        
        // Send the LMP message
        err = lmp_chan_send9(chan->lc,
                             LMP_SEND_FLAGS_DEFAULT,
                             NULL_CAP,
                             LMP_RequestType_StringShort,
                             ((uintptr_t *)string_arg)[0],
                             ((uintptr_t *)string_arg)[1],
                             ((uintptr_t *)string_arg)[2],
                             ((uintptr_t *)string_arg)[3],
                             ((uintptr_t *)string_arg)[4],
                             ((uintptr_t *)string_arg)[5],
                             ((uintptr_t *)string_arg)[6],
                             ((uintptr_t *)string_arg)[7]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            free(string_arg);
            return err;
        }
        
        // Free the memory for constructing the arguments
        free(string_arg);
        
        // Receive the status code form recipient
        struct capref cap;
        struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
        lmp_client_recv(chan->lc, &cap, &msg);
        
        // Check we actually got a valid response
        assert(msg.words[0] == LMP_RequestType_StringShort);
        
        // Return an error if things didn't work
        if (err_is_fail(msg.words[1])) {
            return msg.words[1];
        }
        
        // Return the status code
        return msg.words[2] == len ? SYS_ERR_OK : -1;
        
    }
    else {
        
        /* StringLong */
        
        return 0;
        
    }
    
}

errval_t aos_rpc_get_ram_cap(struct aos_rpc *chan, size_t size, size_t align,
                             struct capref *retcap, size_t *ret_size)
{
    // TODO: implement functionality to request a RAM capability over the
    // given channel and wait until it is delivered.
    
    errval_t err = SYS_ERR_OK;
    
    err = lmp_chan_send3(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryAlloc, size, align);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Initializing message
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the response
    lmp_client_recv(chan->lc, retcap, &msg);
    
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
    // TODO implement functionality to request a character from
    // the serial driver.
    return SYS_ERR_OK;
}


errval_t aos_rpc_serial_putchar(struct aos_rpc *chan, char c)
{
    // TODO implement functionality to send a character to the
    // serial port.
    return SYS_ERR_OK;
}

errval_t aos_rpc_process_spawn(struct aos_rpc *chan, char *name,
                               coreid_t core, domainid_t *newpid)
{
    errval_t err = SYS_ERR_OK;
    
    // Get length of the name string
    size_t len = strlen(name);
    
    // FIXME: Currently the maximum process name size is very limited
    assert(len <= sizeof(uintptr_t) * 7);
    
    // Allocate new memory to construct the arguments
    char *name_arg = calloc(sizeof(uintptr_t), 7);
    
    // Copy in the name string
    memcpy(name_arg, name, len);
    
    // Send the LMP message
    err = lmp_chan_send9(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_Spawn,
                         0,
                         ((uintptr_t *)name_arg)[0],
                         ((uintptr_t *)name_arg)[1],
                         ((uintptr_t *)name_arg)[2],
                         ((uintptr_t *)name_arg)[3],
                         ((uintptr_t *)name_arg)[4],
                         ((uintptr_t *)name_arg)[5],
                         ((uintptr_t *)name_arg)[6]);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        free(name_arg);
        return err;
    }
    
    // Free the memory for constructing the arguments
    free(name_arg);
    
    // Receive the status code and pid form the spawn server
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    lmp_client_recv(chan->lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == LMP_RequestType_Spawn);

    // Return the PID of the new process
    *newpid = msg.words[2];
    
    // Return the status code
    return (errval_t) msg.words[1];
}

errval_t aos_rpc_process_get_name(struct aos_rpc *chan, domainid_t pid,
                                  char **name)
{
    // TODO (milestone 5): implement name lookup for process given a process
    // id
    return SYS_ERR_OK;
}

errval_t aos_rpc_process_get_all_pids(struct aos_rpc *chan,
                                      domainid_t **pids, size_t *pid_count)
{
    // TODO (milestone 5): implement process id discovery
    return SYS_ERR_OK;
}

errval_t aos_rpc_get_device_cap(struct aos_rpc *rpc,
                                lpaddr_t paddr, size_t bytes,
                                struct capref *frame)
{
    return LIB_ERR_NOT_IMPLEMENTED;
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
