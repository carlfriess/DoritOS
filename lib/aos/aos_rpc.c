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
#include <aos/urpc.h>
#include <aos/ump.h>
#include <aos/threads.h>
#include <aos/terminal.h>

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
        lmp_client_recv_waitset(chan->lc, retcap, &msg, &chan->mem_ws);
        
        // Check if we got the message we wanted
        if (msg.words[0] != LMP_RequestType_MemoryAlloc) {

#if PRINT_DEBUG
            debug_printf("Got ack of type: %d\n", msg.words[0]);
#endif
            
            // Allocate a new slot if necessary
            if (!capref_is_null(*retcap)) {
                err = lmp_chan_alloc_recv_slot(chan->lc);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                    return err;
                }
            }
        
            // Request resend
            err = lmp_chan_send9(chan->lc, LMP_SEND_FLAGS_DEFAULT, *retcap, LMP_RequestType_Echo, msg.words[0], msg.words[1], msg.words[2], msg.words[3], msg.words[4], msg.words[5], msg.words[6], msg.words[7]);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
                return err;
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
    errval_t err;

    if (chan->uc == NULL) return LIB_ERR_TERMINAL_INIT;

    size_t size = sizeof(struct terminal_msg);
    urpc_msg_type_t msg_type = URPC_MessageType_TerminalRead;
    struct terminal_msg in;


    err = urpc_send(chan->uc, (void *) &in, size, msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    struct terminal_msg *buf;

    err = urpc_recv_blocking(chan->uc, (void **) &buf, &size, &msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    assert(msg_type == URPC_MessageType_TerminalRead);

    free(buf);

    *retc = buf->c;

    return err;
}


errval_t aos_rpc_serial_putchar(struct aos_rpc *chan, char c)
{
    errval_t err = SYS_ERR_OK;

    if (c == '\n') {
        aos_rpc_serial_putchar(chan, '\r');
    }

    size_t size = sizeof(struct terminal_msg);
    urpc_msg_type_t msg_type = URPC_MessageType_TerminalWrite;
    struct terminal_msg msg = {
        .c = c
    };


    err = urpc_send(chan->uc, (void *) &msg, size, msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    void *out;
    err = urpc_recv_blocking(chan->uc, (void **) &out, &size, &msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    assert(msg_type == URPC_MessageType_TerminalWrite);

    free(out);
    return err;
}

errval_t aos_rpc_process_spawn(struct aos_rpc *chan, char *name,
                               coreid_t core, domainid_t *newpid)
{
    
    return aos_rpc_process_spawn_with_terminal(chan, name, core, 0, newpid);
    
}

errval_t aos_rpc_process_spawn_with_terminal(struct aos_rpc *chan,
                                             char *name,
                                             coreid_t core,
                                             domainid_t terminal_pid,
                                             domainid_t *newpid)
{
    
    errval_t err;

    // Send spawn request with send_short_buf() or send_frame() depending on size of name
    err = lmp_send_spawn(chan->lc, name, core, terminal_pid);
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

errval_t aos_rpc_process_get_pid_by_name(const char *name, domainid_t *pid) {
    
    errval_t err;
    
    *pid = 0;
    
    struct aos_rpc *rpc_chan = aos_rpc_get_init_channel();
    
    // Request the pids of all running processes
    domainid_t *pids;
    size_t num_pids;
    err = aos_rpc_process_get_all_pids(rpc_chan, &pids, &num_pids);
    if(err_is_fail(err)) {
        return err;
    }

    
    // Iterate pids and compare names
    for (int i = 0; i < num_pids; i++) {
        
        // Get the process name
        char *process_name;
        err = aos_rpc_process_get_name(rpc_chan, pids[i], &process_name);
        if(err_is_fail(err)) {
            debug_printf("aos_rpc_process_get_pid_by_name(): %s\n", err_getstring(err));
            continue;
        }
        
        // Compare the name
        if (!strcmp(name, process_name)) {
            *pid = pids[i];
            free(process_name);
            break;
        }
        free(process_name);
    }
    
    // Make sure the process was found
    if (*pid == 0) {
        return LIB_ERR_PID_NOT_FOUND;
    }
    
    return SYS_ERR_OK;
    
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
    
    // Allocate recv slot
    err = lmp_chan_alloc_recv_slot(chan->lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    struct frame_identity id;
    invoke_frame_identify(*frame, &id);
    
    //debug_printf("Received %llx %llu\n", id.base, id.bytes);
    
    //debug_printf("%s\n", err_getstring((errval_t) msg.words[1]));
    
    return err;
    
}

errval_t aos_rpc_get_module_list(struct aos_rpc *chan,
                                 char ***modules,
                                 size_t *module_count)
{

    errval_t err;
    
    assert(modules != NULL);
    
    // Send request to get frame buffer with all multiboot module names
    err = lmp_chan_send1(chan->lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_ModuleList
                         );
    
    // Buffer for names
    char *buffer;
    
    // Buffer size
    size_t buffer_size = 0;
    
    // Message type
    uint8_t msg_type;
    
    // Receive buffer with all module names from init
    lmp_recv_buffer(chan->lc, (void **) &buffer, &buffer_size, &msg_type);
    
    assert(msg_type == LMP_RequestType_ModuleList);

    // Get module count from buffer
    size_t count = ((uint32_t *) buffer)[0];
    
    // Array of all module names
    char **modules_array = calloc(count, sizeof(char *));
    
    // Pointer to beginning of current name string in buffer
    char *ptr = buffer + sizeof(uint32_t);
    char *next_ptr;
    
    size_t i;
    for (i = 0; i < count; i++) {
        
        next_ptr = strchr(ptr, '\0');
        if (next_ptr == NULL) {
            debug_printf("Failed parsing module name buffer\n");
            break;
        }
        
        // Set ith element in modules name array
        modules_array[i] = strdup(ptr);
        
        // Update pointer
        ptr = next_ptr + 1;
        
    }
    
    // Set return module count
    *module_count = i;
    
    // Set return modules array
    *modules = modules_array;
    
    // Free buffer
    free(buffer);

    return err;
    
}

errval_t aos_rpc_get_module_frame(struct aos_rpc *chan, char *name,
                                  struct capref *frame, size_t *ret_bytes) {
    
    errval_t err;
    
    assert(ret_bytes != NULL);
    
    // Allocating frame capability
    size_t ret_size;
    struct capref frame_cap;
    err = frame_alloc(&frame_cap, strlen(name) + 1, &ret_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Mapping frame into virtual address space
    void *buf;
    err = paging_map_frame(get_current_paging_state(), &buf,
                           ret_size, frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Copy name into buffer after core id
    memcpy(buf, name, strlen(name) + 1);
    
    // Send the frame to the recipient
    err = lmp_send_frame(chan->lc, LMP_RequestType_ModuleFrame, frame_cap, ret_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Cleaning up after sending
    err = paging_unmap(get_current_paging_state(), buf);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    // FIXME: Make this work
    /*err = ram_free_handler(frame_cap, ret_size);
     if (err_is_fail(err)) {
     debug_printf("%s\n", err_getstring(err));
     }*/
    // Temporary less optimal soution:
    cap_delete(frame_cap);
    slot_free(frame_cap);
    

    // Initialize message
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the frame capability where the module names are saved
    lmp_client_recv(chan->lc, frame, &msg);
    
    // Allocate recv slot
    err = lmp_chan_alloc_recv_slot(chan->lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Check that it received correct response
    assert(msg.words[0] == LMP_RequestType_ModuleFrame);
    
    // Set error
    err = msg.words[1];
    
    // Set return bytes
    *ret_bytes = msg.words[2];
    
    return err;
    
}


errval_t aos_rpc_process_deregister(void) {
    errval_t err;

    struct aos_rpc *chan = aos_rpc_get_init_channel();

    struct aos_rpc *serial_chan = aos_rpc_get_serial_channel();

    void *buf = NULL;

    size_t size = sizeof(void *);
    urpc_msg_type_t msg_type = URPC_MessageType_TerminalDeregister;

    err = urpc_send(serial_chan->uc, (void *) &buf, size, msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    err = urpc_recv_blocking(serial_chan->uc, (void **) &buf, &size, &msg_type);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    free(buf);

    lmp_chan_send1(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_ProcessDeregister);

    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    struct capref cap;

    lmp_client_recv(chan->lc, &cap, &msg);

    return SYS_ERR_OK;
}

errval_t aos_rpc_process_deregister_notify(domainid_t pid) {
    struct aos_rpc *chan = aos_rpc_get_init_channel();

    lmp_chan_send2(chan->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_ProcessDeregisterNotify, pid);

    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    struct capref cap;
    lmp_client_recv(chan->lc, &cap, &msg);

    return SYS_ERR_OK;
}

errval_t aos_rpc_init(struct aos_rpc *rpc, struct lmp_chan *lc)
{
    errval_t err = SYS_ERR_OK;

    // TODO: Initialize given rpc channel
    rpc->lc = lc;
    waitset_init(&rpc->mem_ws);

    if (!strcmp(disp_name(), "terminal")) return err;

    rpc->uc = (struct urpc_chan *) malloc(sizeof(struct urpc_chan));
    assert(rpc->uc != NULL);

    set_init_rpc(rpc);
    
    domainid_t pid;
    err = aos_rpc_process_get_pid_by_name("terminal", &pid);
    assert(err_is_ok(err));
    
    err = urpc_bind(disp_get_terminal_pid(),
                    rpc->uc,
                    (pid != disp_get_terminal_pid() ? 1 : !disp_get_core_id()));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    return err;
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
