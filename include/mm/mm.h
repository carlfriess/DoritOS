/**
 * \file
 * \brief Memory manager header
 */

/*
 * Copyright (c) 2008, 2011, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef AOS_MM_H
#define AOS_MM_H

#include <sys/cdefs.h>
#include <errors/errno.h>
#include <aos/types.h>
#include <aos/capabilities.h>
#include <aos/slab.h>
#include "slot_alloc.h"

__BEGIN_DECLS

enum nodetype {
    NodeType_Free,      ///< This region exists and is free
    NodeType_Allocated  ///< This region exists and is allocated
};

struct capinfo {
    struct capref cap;
    genpaddr_t base;
    gensize_t size;
};

/**
 * \brief Node in Memory manager
 */
struct mmnode {
    enum nodetype type;    ///< Type of `this` node.
    struct capinfo cap;    ///< Cap in which this region exists
    struct mmnode *prev;   ///< Previous node in the list.
    struct mmnode *next;   ///< Next node in the list.
    genpaddr_t base;       ///< Base address of this region
    gensize_t size;        ///< Size of this free region in cap
};

/**
 * \brief Memory manager instance data
 *
 * This should be opaque from the perspective of the client, but to allow
 * them to allocate its memory, we declare it in the public header.
 */
struct mm {
    struct slab_allocator slabs; ///< Slab allocator used for allocating nodes
    slot_alloc_t slot_alloc;     ///< Slot allocator for allocating cspace
    slot_refill_t slot_refill;   ///< Slot allocator refill function
    void *slot_alloc_inst;       ///< Opaque instance pointer for slot allocator
    enum objtype objtype;        ///< Type of capabilities stored
    struct mmnode *head;         ///< Head of doubly-linked list of nodes in order
};

errval_t mm_init(struct mm *mm, enum objtype objtype,
                     slab_refill_func_t slab_refill_func,
                     slot_alloc_t slot_alloc_func,
                     slot_refill_t slot_refill_func,
                     void *slot_alloc_inst);
errval_t mm_add(struct mm *mm, struct capref cap, genpaddr_t base, size_t size);
errval_t mm_alloc_aligned(struct mm *mm, size_t size, size_t alignment,
                              struct capref *retcap);
errval_t mm_alloc(struct mm *mm, size_t size, struct capref *retcap);
errval_t mm_free(struct mm *mm, struct capref cap, genpaddr_t base, gensize_t size);
void mm_dump_mmnodes(struct mm *mm);
void mm_destroy(struct mm *mm);

__END_DECLS

#endif /* AOS_MM_H */
