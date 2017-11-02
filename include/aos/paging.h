/**
 * \file
 * \brief Barrelfish paging helpers.
 */

/*
 * Copyright (c) 2012, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */


#ifndef LIBBARRELFISH_PAGING_H
#define LIBBARRELFISH_PAGING_H

#include <errors/errno.h>
#include <aos/capabilities.h>
#include <aos/slab.h>
#include <aos/except.h>
#include <barrelfish_kpi/paging_arm_v7.h>

typedef int paging_flags_t;

#define VADDR_OFFSET ((lvaddr_t)1UL*1024*1024*1024) // 1GB

#define PAGING_SLAB_BUFSIZE 12

#define VREGION_FLAGS_READ     0x01 // Reading allowed
#define VREGION_FLAGS_WRITE    0x02 // Writing allowed
#define VREGION_FLAGS_EXECUTE  0x04 // Execute allowed
#define VREGION_FLAGS_NOCACHE  0x08 // Caching disabled
#define VREGION_FLAGS_MPB      0x10 // Message passing buffer
#define VREGION_FLAGS_GUARD    0x20 // Guard page
#define VREGION_FLAGS_MASK     0x2f // Mask of all individual VREGION_FLAGS

#define VREGION_FLAGS_READ_WRITE \
    (VREGION_FLAGS_READ | VREGION_FLAGS_WRITE)
#define VREGION_FLAGS_READ_EXECUTE \
    (VREGION_FLAGS_READ | VREGION_FLAGS_EXECUTE)
#define VREGION_FLAGS_READ_WRITE_NOCACHE \
    (VREGION_FLAGS_READ | VREGION_FLAGS_WRITE | VREGION_FLAGS_NOCACHE)
#define VREGION_FLAGS_READ_WRITE_MPB \
    (VREGION_FLAGS_READ | VREGION_FLAGS_WRITE | VREGION_FLAGS_MPB)

struct paging_region {
    lvaddr_t base_addr;
    lvaddr_t current_addr;
    size_t region_size;
    // TODO: if needed add struct members for tracking state
    void *paging_state;
};

// struct to store the paging status of a process
struct paging_state {
    struct slot_allocator* slot_alloc;
    // TODO: add struct members to keep track of the page tables etc
    struct capref l1_pagetable;
    struct slab_allocator vspace_slabs;             // Slab allocator for free_vspace_node
    int vspace_slabs_prevent_refill;                // Keep track when to prevent refill
    struct vspace_node *alloc_vspace_head;          // Alloc list of allocated vspace regions
    struct vspace_node *free_vspace_head;           // Free list of free vspace regions
    lvaddr_t free_vspace_base;                      // Base address of free vspace
    struct slab_allocator slabs;                    // Slab allocator for pt_cap_tree_node
    int slabs_prevent_refill;                       // Keep track when to prevent refill
    struct pt_cap_tree_node *l2_tree_root;          // Tree of all L2 page table caps with subtrees for mappings
    struct paging_region exception_stack_region;    // Paging region for the exception handler's stack
};

// struct for list of free virtual address regions
struct vspace_node {
    struct vspace_node *next;
    lvaddr_t base;
    size_t size;
};

// struct for tree of allocated l2_pagetable capabilities
struct pt_cap_tree_node {
    struct pt_cap_tree_node *left;
    struct pt_cap_tree_node *right;
    struct pt_cap_tree_node *subtree;
    uintptr_t offset;
    struct capref cap;
    struct capref mapping_cap;
};

struct thread;

void exception_handler(enum exception_type type, int subtype, void *addr, arch_registers_state_t *regs, arch_registers_fpu_state_t *fpuregs);

/// Initialize paging_state struct
errval_t paging_init_state(struct paging_state *st, lvaddr_t start_vaddr,
        struct capref pdir, struct slot_allocator * ca);
/// initialize self-paging module
errval_t paging_init(void);
/// setup paging on new thread (used for user-level threads)
void paging_init_onthread(struct thread *t);


errval_t paging_region_init(struct paging_state *st,
                            struct paging_region *pr, size_t size);

/**
 * \brief return a pointer to a bit of the paging region `pr`.
 * This function gets used in some of the code that is responsible
 * for allocating Frame (and other) capabilities.
 */
errval_t paging_region_map(struct paging_region *pr, size_t req_size,
                           void **retbuf, size_t *ret_size);
/**
 * \brief free a bit of the paging region `pr`.
 * This function gets used in some of the code that is responsible
 * for allocating Frame (and other) capabilities.
 * We ignore unmap requests right now.
 */
errval_t paging_region_unmap(struct paging_region *pr, lvaddr_t base, size_t bytes);

/**
 * \brief Allocate a fixed area in the virtual address space.
 */
errval_t paging_alloc_fixed(struct paging_state *st, void *buf, size_t bytes);

errval_t paging_alloc_fixed_commit(struct paging_state *st);

/**
 * \brief Find a bit of free virtual address space that is large enough to
 *        accomodate a buffer of size `bytes`.
 */
errval_t paging_alloc(struct paging_state *st, void **buf, size_t bytes);

/**
 * Functions to map a user provided frame.
 */
/// Map user provided frame with given flags while allocating VA space for it
errval_t paging_map_frame_attr(struct paging_state *st, void **buf,
                               size_t bytes, struct capref frame,
                               int flags, void *arg1, void *arg2);
/// Map user provided frame at user provided VA with given flags.
errval_t paging_map_fixed_attr(struct paging_state *st, lvaddr_t vaddr,
                               struct capref frame, size_t bytes, int flags);

/**
 * refill slab allocator without causing a page fault
 */
errval_t slab_refill_no_pagefault(struct slab_allocator *slabs,
                                  struct capref frame, size_t minbytes);

/**
 * \brief unmap region starting at address `region`.
 * NOTE: this function is currently here to make libbarrelfish compile. As
 * noted on paging_region_unmap we ignore unmap requests right now.
 */
errval_t paging_unmap(struct paging_state *st, const void *region);

errval_t paging_free(struct paging_state *st, const void *region, size_t *ret_size);

errval_t paging_unmap_fixed(struct paging_state *st, lvaddr_t vaddr, size_t bytes);


/// Map user provided frame while allocating VA space for it
static inline errval_t paging_map_frame(struct paging_state *st, void **buf,
                                        size_t bytes, struct capref frame,
                                        void *arg1, void *arg2)
{
    return paging_map_frame_attr(st, buf, bytes, frame,
            VREGION_FLAGS_READ_WRITE, arg1, arg2);
}

/// Map user provided frame at user provided VA.
static inline errval_t paging_map_fixed(struct paging_state *st, lvaddr_t vaddr,
                                        struct capref frame, size_t bytes)
{
    return paging_map_fixed_attr(st, vaddr, frame, bytes,
            VREGION_FLAGS_READ_WRITE);
}

static inline lvaddr_t paging_genvaddr_to_lvaddr(genvaddr_t genvaddr) {
    return (lvaddr_t) genvaddr;
}

#endif // LIBBARRELFISH_PAGING_H
