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

struct parent_mapping {
    struct parent_mapping *next;
    void *addr;
};

struct spawninfo {

    // Information about the binary
    char * binary_name;     // Name of the binary

    // TODO: Use this structure to keep track
    // of information you need for building/starting
    // your new process!

    struct paging_state child_paging_state; // Child's paging state
    void *got_addr;                     // Address of the global offset table
    genvaddr_t entry_addr;              // Entry address to child program
    void *dcb_addr_parent;              // Address of DCB (parent's vspace)
    struct parent_mapping parent_mappings;              // Unneeded paging regions

    // Child's capabilites
    struct cnoderef taskcn_ref;         // TASKCN
    struct capref slot_rootcn_cap;      // Root cnode capability
    struct cnoderef slot_pagecn_ref;    // SLOT_PAGECN
    struct capref l1_pt_cap;            // L1 pagetable capability
    struct capref slot_dispframe_cap;   // Capability to the dispatcher frame

    // Parent's capabilities
    struct capref child_rootcn_cap;     // Capability to the child's L1 cnode
    struct capref child_root_pt_cap;    // Capability to the child's L1 pagetable
    struct capref child_dispatcher_cap; // Capability to the child's dispatcher

};

struct process_info {
    struct process_info *next;
    struct process_info *prev;
    char *name;
    struct capref *dispatcher_cap;
};


// Start a child process by binary name. Fills in si
errval_t spawn_load_by_name(void * binary_name, struct spawninfo * si);

#endif /* _INIT_SPAWN_H_ */
