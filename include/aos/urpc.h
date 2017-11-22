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

void urpc_init_server_handler(struct ump_chan *chan, void *msg, size_t size,
                              ump_msg_type_t msg_type);

#endif /* urpc_h */
