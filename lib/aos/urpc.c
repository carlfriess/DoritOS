//
//  urpc.c
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <aos/aos.h>
#include <aos/aos_rpc.h>
#include <aos/lmp.h>
#include <aos/urpc_protocol.h>
#include <spawn_serv.h>


#include "aos/urpc.h"

extern struct ump_chan init_uc;
extern lmp_server_spawn_handler lmp_server_spawn_handler_func;

// MARK: - Init URPC Server

static void urpc_spawn_handler(struct ump_chan *chan, void *msg, size_t size,
                               ump_msg_type_t msg_type);
static void urpc_get_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type);

// Handler for init URPC server:
void urpc_init_server_handler(struct ump_chan *chan, void *msg, size_t size,
                              ump_msg_type_t msg_type) {
    
    switch (msg_type) {
            
        case UMP_MessageType_Spawn:
            urpc_spawn_handler(chan, msg, size, msg_type);
            break;
            
        case UMP_MessageType_TerminalGetChar:
            urpc_get_char_handler(chan, msg, size, msg_type);
            break;
            
        case UMP_MessageType_RegisterProcess:
            urpc_register_process_handler(chan, msg, size, msg_type);
            break;
            
        default:
            USER_PANIC("Unknown UMP message type\n");
            break;
            
    }
    
}

// Handle UMP_MessageType_Spawn
static void urpc_spawn_handler(struct ump_chan *chan, void *msg, size_t size,
                               ump_msg_type_t msg_type) {
    
    struct urpc_spaw_response res;
    
    // Pass message to spawn server
    res.err = lmp_server_spawn_handler_func((char *) msg, disp_get_core_id(), &res.pid);
    
    // Send response back to requesting core
    ump_send(chan,
             (void *) &res,
             sizeof(struct urpc_spaw_response),
             UMP_MessageType_SpawnAck);
    
}

// Handle UMP_MessageType_TerminalGetChar
static void urpc_get_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type) {
    
    char c;
    do {
        sys_getchar(&c);
    } while (c == '\0');
    
    ump_send(chan, &c, sizeof(char), UMP_MessageType_TerminalGetCharAck);
    
}

// Handle UMP_MessageType_RegisterProcess
void urpc_register_process_handler(struct ump_chan *chan, void *msg,
                                   size_t size, ump_msg_type_t msg_type) {
    
    struct urpc_process_register *req = msg;
    struct process_info *new_process = malloc(sizeof(struct process_info));
    assert(new_process != NULL);
    
    new_process->core_id = req->core_id;
    new_process->pid = req->pid;
    new_process->name = (char *) malloc(strlen(req->name));
    assert(new_process->name != NULL);
    strcpy(new_process->name, req->name);
    
    process_register(new_process);
    
    ump_send_one(chan,
                 &new_process->pid,
                 sizeof(domainid_t),
                 UMP_MessageType_RegisterProcessAck,
                 1);
    
}



// MARK: - Init URPC Client

// RPC for registering a process
void urpc_process_register(struct process_info *pi) {
    
    size_t msg_size = sizeof(struct urpc_process_register) + strlen(pi->name) + 1;
    struct urpc_process_register *msg = (struct urpc_process_register *) malloc(msg_size);
    assert(msg != NULL);
    
    msg->core_id = pi->core_id;
    msg->pid = pi->pid;
    strcpy(msg->name, pi->name);
    
    ump_send(&init_uc, (void *)msg, msg_size, UMP_MessageType_RegisterProcess);
    
    free(msg);
    
    domainid_t *pid = malloc(UMP_SLOT_DATA_BYTES);
    
    ump_msg_type_t msg_type;
    uint8_t last;
    errval_t err;
    do {
        err = ump_recv_one(&init_uc, (void *)pid, &msg_type, &last);
    } while (err == LIB_ERR_NO_UMP_MSG);
    assert(err_is_ok(err));
    assert(msg_type == UMP_MessageType_RegisterProcessAck);
    assert(last == 1);
    
    pi->pid = *pid;
    
    free(pid);
    
}



// MARK: - Generic Server



// MARK: - Generic Client

static struct lmp_chan *get_init_lmp_chan(void) {
    return get_init_rpc()->lc;
}

// Bind to a URPC server with a specific PID
errval_t urpc_bind(domainid_t pid, struct ump_chan *chan) {
    
    errval_t err;
    
    // Get the channel to this core's init
    struct lmp_chan *lc = get_init_lmp_chan();
    
    // Allocate a frame for the new UMP channel
    struct capref ump_frame_cap;
    size_t ump_frame_size = UMP_BUF_SIZE;
    err = frame_alloc(&ump_frame_cap, ump_frame_size, &ump_frame_size);
    assert(ump_frame_size >= UMP_BUF_SIZE);
    
    // Send a bind request with the frame capability to this core's init
    lmp_chan_send2(lc,
                   LMP_SEND_FLAGS_DEFAULT,
                   ump_frame_cap,
                   LMP_RequestType_UmpBind,
                   pid);
    
    // Initialize the UMP channel
    ump_chan_init(chan, UMP_CLIENT_BUF_SELECT);
    
    // Map the allocated UMP frame
    err = paging_map_frame(get_current_paging_state(), (void **) &chan->buf,
                           ump_frame_size, ump_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Get the frame identity
    err = frame_identify(ump_frame_cap, &chan->fi);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Wait for ack from server
    errval_t *reterr;
    size_t retsize;
    ump_msg_type_t msg_type;
    do {
        err = ump_recv(chan, (void **) &reterr, &retsize, &msg_type);
    }
    while (err == LIB_ERR_NO_UMP_MSG);
    
    // Check we recieved the correct message
    assert(msg_type == UMP_MessageType_UrpcBindAck);
    
    return *reterr;
    
}



// MARK: - URPC bind handlers

static void urpc_forward_request_ump(void) {
    // TODO
}

static void urpc_forward_request_lmp(void) {
    // TODO
}

// Handle a URPC bind request received via LMP
void urpc_handle_lmp_bind_request(void) {
    urpc_forward_request_ump();
    urpc_forward_request_lmp();
}
