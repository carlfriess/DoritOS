//
//  spawn_serv.h
//  DoritOS
//
//  Created by Carl Friess on 27/10/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef SPAWN_SERV_H
#define SPAWN_SERV_H

#include <aos/urpc.h>

errval_t spawn_serv_init(struct urpc_chan *chan);

#endif /* SPAWN_SERV_H */
