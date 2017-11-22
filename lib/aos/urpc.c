//
//  urpc.c
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <aos/aos.h>
#include <aos/urpc_protocol.h>
#include <spawn_serv.h>


#include "aos/urpc.h"

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
    res.err = spawn_serv_handler((char *) msg, disp_get_core_id(), &res.pid);
    
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
