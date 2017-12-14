/*
 * Copyright (c) 2012-2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef DISTOPS_DELETESTEP_H
#define DISTOPS_DELETESTEP_H

#include <aos/caddr.h>
#include <aos/waitset.h>
#include <aos/event_queue.h>
#include "distops/domcap.h"

struct waitset *delete_steps_get_waitset(void);
void delete_steps_init(struct waitset *ws);
void delete_steps_trigger(void);
void delete_steps_pause(void);
void delete_steps_resume(void);

struct delete_queue_node {
    struct event_queue_node qn;
    struct delete_queue_node *next;
    struct event_closure cont;
};

void delete_queue_wait(struct delete_queue_node *qn,
                       struct event_closure cont);

#endif
