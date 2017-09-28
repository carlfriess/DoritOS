/**
 * \file
 * \brief create child process library
 */

/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef _INIT_SPAWN_H_
#define _INIT_SPAWN_H_

#include "aos/slot_alloc.h"
#include "aos/paging.h"

struct spawninfo {

    // Information about the binary
    char * binary_name;     // Name of the binary

    // TODO: Use this structure to keep track
    // of information you need for building/starting
    // your new process!
};

// Start a child process by binary name. Fills in si
errval_t spawn_load_by_name(void * binary_name, struct spawninfo * si);

#endif /* _INIT_SPAWN_H_ */
