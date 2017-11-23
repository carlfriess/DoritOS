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


#endif /* urpc_h */
