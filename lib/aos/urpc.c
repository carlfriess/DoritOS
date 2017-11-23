//
//  urpc.c
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <aos/aos.h>
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
static void urpc_register_process_handler(struct ump_chan *chan, void *msg,
                                          size_t size, ump_msg_type_t msg_type);

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
static void urpc_register_process_handler(struct ump_chan *chan, void *msg,
                                          size_t size, ump_msg_type_t msg_type) {
    
    struct urpc_process_register *req = msg;
    struct process_info *new_process = malloc(sizeof(struct process_info));
    assert(new_process != NULL);
    
    new_process->core_id = req->core_id;
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

