//
//  urpc.h
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef urpc_h
#define urpc_h

#include <aos/ump.h>
#include <aos/process.h>

#include <stdbool.h>


#define URPC_MessageType_UrpcBindAck    UMP_MessageType_UrpcBindAck

#define URPC_MessageType_User0  UMP_MessageType_User0
#define URPC_MessageType_User1  UMP_MessageType_User1
#define URPC_MessageType_User2  UMP_MessageType_User2
#define URPC_MessageType_User3  UMP_MessageType_User3
#define URPC_MessageType_User4  UMP_MessageType_User4
#define URPC_MessageType_User5  UMP_MessageType_User5
#define URPC_MessageType_User6  UMP_MessageType_User6
#define URPC_MessageType_User7  UMP_MessageType_User7


// UMP message types type
typedef ump_msg_type_t urpc_msg_type_t;

// URPC channel
struct urpc_chan {
    bool use_lmp;
    struct ump_chan *ump;
    struct lmp_chan *lmp;
};


// MARK: - Init URPC Server

// Handler for init URPC server:
void urpc_init_server_handler(struct ump_chan *chan, void *msg, size_t size,
                              ump_msg_type_t msg_type);

// Handle UMP_MessageType_RegisterProcess
void urpc_register_process_handler(struct ump_chan *chan, void *msg,
                                   size_t size, ump_msg_type_t msg_type);

// MARK: - Init URPC Client

// RPC for registering a process
void urpc_process_register(struct process_info *pi);


// MARK: - Generic Server

// Accept a bind request and set up the URPC channel
errval_t urpc_accept(struct urpc_chan *chan);


// MARK: - Generic Client

// Bind to a URPC server with a specific PID
errval_t urpc_bind(domainid_t pid, struct urpc_chan *chan, bool use_lmp);


// MARK: - Generic Send & Receive

// Send on a URPC channel
errval_t urpc_send(struct urpc_chan *chan, void *buf, size_t size,
                   urpc_msg_type_t msg_type);

// Receive on a URPC channel
errval_t urpc_recv(struct urpc_chan *chan, void **buf, size_t *size,
                   urpc_msg_type_t* msg_type);

// Blockingly receive on a URPC channel
errval_t urpc_recv_blocking(struct urpc_chan *chan, void **buf, size_t *size,
                            urpc_msg_type_t* msg_type);


// MARK: - URPC bind handlers

// Handle a URPC bind request received via LMP
void urpc_handle_lmp_bind_request(struct lmp_chan *lc,
                                  struct capref msg_cap,
                                  struct lmp_recv_msg msg);

// Handle a URPC bind request received via UMP
void urpc_handle_ump_bind_request(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type);

#endif /* urpc_h */
