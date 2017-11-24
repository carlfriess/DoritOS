//
//  spawn_serv.h
//  DoritOS
//
//  Created by Carl Friess on 27/10/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef SPAWN_SERV_H
#define SPAWN_SERV_H

#include <aos/ump.h>

errval_t spawn_serv_handler(char *name, coreid_t coreid, domainid_t *pid);

errval_t spawn_serv_init(struct ump_chan *chan);

#endif /* SPAWN_SERV_H */
