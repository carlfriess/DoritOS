/**
 * \file
 * \brief AOS paging helpers.
 */

/*
 * Copyright (c) 2012, 2013, 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos.h>
#include <aos/paging.h>
#include <aos/except.h>
#include <aos/slab.h>
#include "threads_priv.h"

#include <stdio.h>
#include <string.h>

static struct paging_state current;

/**
 * \brief Helper function that allocates a slot and
 *        creates a ARM l2 page table capability
 */
__attribute__((unused))
static errval_t arml2_alloc(struct paging_state * st, struct capref *ret)
{
    errval_t err;
    err = st->slot_alloc->alloc(st->slot_alloc, ret);
    if (err_is_fail(err)) {
        debug_printf("slot_alloc failed: %s\n", err_getstring(err));
        return err;
    }
    err = vnode_create(*ret, ObjType_VNode_ARM_l2);
    if (err_is_fail(err)) {
        debug_printf("vnode_create failed: %s\n", err_getstring(err));
        return err;
    }
    return SYS_ERR_OK;
}

errval_t paging_init_state(struct paging_state *st, lvaddr_t start_vaddr,
        struct capref pdir, struct slot_allocator * ca)
{
    debug_printf("paging_init_state\n");
    // TODO (M2): implement state struct initialization
    // TODO (M4): Implement page fault handler that installs frames when a page fault
    // occurs and keeps track of the virtual address space.
    return SYS_ERR_OK;
}

/**
 * \brief This function initializes the paging for this domain
 * It is called once before main.
 */
errval_t paging_init(void)
{
    debug_printf("paging_init\n");
    // TODO (M2): Call paging_init_state for &current
    // TODO (M4): initialize self-paging handler
    // TIP: use thread_set_exception_handler() to setup a page fault handler
    // TIP: Think about the fact that later on, you'll have to make sure that
    // you can handle page faults in any thread of a domain.
    // TIP: it might be a good idea to call paging_init_state() from here to
    // avoid code duplication.
    set_current_paging_state(&current);
    
    // Storing the reference to the default slot allocator
    current.slot_alloc = get_default_slot_allocator();
    
    // Set the capability reference for the l1 page table to the default location in capability space
    current.l1_pagetable.cnode = cnode_page;
    current.l1_pagetable.slot = 0;
    
    // Initialize the slab allocator for tree nodes
    slab_init(&current.slabs, sizeof(struct pt_cap_tree_node), slab_default_refill);
    static char nodebuf[sizeof(struct pt_cap_tree_node)*64];
    slab_grow(&current.slabs, nodebuf, sizeof(nodebuf));
    
    return SYS_ERR_OK;
}


/**
 * \brief Initialize per-thread paging state
 */
void paging_init_onthread(struct thread *t)
{
    // TODO (M4): setup exception handler for thread `t'.
}

/**
 * \brief return a pointer to a bit of the paging region `pr`.
 * This function gets used in some of the code that is responsible
 * for allocating Frame (and other) capabilities.
 */
errval_t paging_region_init(struct paging_state *st, struct paging_region *pr, size_t size)
{
    void *base;
    errval_t err = paging_alloc(st, &base, size);
    if (err_is_fail(err)) {
        debug_printf("paging_region_init: paging_alloc failed\n");
        return err_push(err, LIB_ERR_VSPACE_MMU_AWARE_INIT);
    }
    pr->base_addr    = (lvaddr_t)base;
    pr->current_addr = pr->base_addr;
    pr->region_size  = size;
    // TODO: maybe add paging regions to paging state?
    return SYS_ERR_OK;
}

/**
 * \brief return a pointer to a bit of the paging region `pr`.
 * This function gets used in some of the code that is responsible
 * for allocating Frame (and other) capabilities.
 */
errval_t paging_region_map(struct paging_region *pr, size_t req_size,
                           void **retbuf, size_t *ret_size)
{
    lvaddr_t end_addr = pr->base_addr + pr->region_size;
    ssize_t rem = end_addr - pr->current_addr;
    if (rem > req_size) {
        // ok
        *retbuf = (void*)pr->current_addr;
        *ret_size = req_size;
        pr->current_addr += req_size;
    } else if (rem > 0) {
        *retbuf = (void*)pr->current_addr;
        *ret_size = rem;
        pr->current_addr += rem;
        debug_printf("exhausted paging region, "
                "expect badness on next allocation\n");
    } else {
        return LIB_ERR_VSPACE_MMU_AWARE_NO_SPACE;
    }
    return SYS_ERR_OK;
}

/**
 * \brief free a bit of the paging region `pr`.
 * This function gets used in some of the code that is responsible
 * for allocating Frame (and other) capabilities.
 * NOTE: Implementing this function is optional.
 */
errval_t paging_region_unmap(struct paging_region *pr, lvaddr_t base, size_t bytes)
{
    // XXX: should free up some space in paging region, however need to track
    //      holes for non-trivial case
    return SYS_ERR_OK;
}

/**
 * TODO(M2): Implement this function
 * \brief Find a bit of free virtual address space that is large enough to
 *        accomodate a buffer of size `bytes`.
 */
errval_t paging_alloc(struct paging_state *st, void **buf, size_t bytes)
{
    *buf = NULL;
    return SYS_ERR_OK;
}

/**
 * \brief map a user provided frame, and return the VA of the mapped
 *        frame in `buf`.
 */
errval_t paging_map_frame_attr(struct paging_state *st, void **buf,
                               size_t bytes, struct capref frame,
                               int flags, void *arg1, void *arg2)
{
    errval_t err = paging_alloc(st, buf, bytes);
    if (err_is_fail(err)) {
        return err;
    }
    return paging_map_fixed_attr(st, (lvaddr_t)(*buf), frame, bytes, flags);
}

errval_t
slab_refill_no_pagefault(struct slab_allocator *slabs, struct capref frame, size_t minbytes)
{
    // Refill the two-level slot allocator without causing a page-fault
    
    // Free the capability slot that was given to us, as we don't use it.
    slot_free(frame);
    
    // FIXME: Currently a full page is allocated. More is not supported.
    assert(minbytes <= BASE_PAGE_SIZE);
    
    // Perform the refill. FIXME: This should not cause a page fault. Hopefully.
    slab_default_refill(slabs);
    
    return SYS_ERR_OK;
}

/**
 * \brief map a user provided frame at user provided VA.
 * TODO(M1): Map a frame assuming all mappings will fit into one L2 pt
 * TODO(M2): General case 
 */
errval_t paging_map_fixed_attr(struct paging_state *st, lvaddr_t vaddr,
        struct capref frame, size_t bytes, int flags)
{
    uintptr_t addr = vaddr;
    
    debug_printf("Mapping page at 0x%x\n", addr);
    
    // Calculate the offsets for the given virtual address
    uintptr_t l1_offset = ARM_L1_OFFSET(addr);
    uintptr_t l2_offset = ARM_L2_OFFSET(addr);
    uintptr_t mapping_offset = addr % BASE_PAGE_SIZE;
    
    // Search for L2 pagetable capability in the tree
    struct pt_cap_tree_node *node = st->l2_tree_root;
    struct pt_cap_tree_node *prev = node;
    debug_printf("+");

    while (node != NULL) {
        if (l1_offset == node->offset) {
            break;
        }
        prev = node;
        if (l1_offset < node->offset) {
            node = node->left;
        }
        else if (l1_offset > node->offset) {
            node = node->right;
        }
    }
    
    // Create a L2 pagetable capability node if it wasn't found
    if (node == NULL) {
        
        // Allocate the new tree node
        node = slab_alloc(&current.slabs);
        
        // Allocate a new slot for the mapping capability
        errval_t err_slot_alloc = slot_alloc(&node->mapping_cap);
        if (!err_is_ok(err_slot_alloc)) {
            return err_slot_alloc;
        }
        
        // Allocate a new L2 pagetable and get the capability
        errval_t err_l2_alloc = arml2_alloc(st, &node->cap);
        if (!err_is_ok(err_l2_alloc)) {
            slot_free(node->mapping_cap);
            return err_l2_alloc;
        }
    
        // Map L2 pagetable to appropriate slote in L1 pagetable
        //  TODO: (M2) Frames going over multiple l2_pagetables
        errval_t err_l2_map = vnode_map(current.l1_pagetable, node->cap, l1_offset, flags, 0, 1, node->mapping_cap);
        if (!err_is_ok(err_l2_map)) {
            slot_free(node->mapping_cap);
            return err_l2_map;
        }
        
        // Set the offset for the new node
        node->offset = l1_offset;
        
        // Store new node in the tree
        if (st->l2_tree_root == NULL) {
            st->l2_tree_root = node;
        }
        else if (prev->offset > l1_offset) {
            prev->left = node;
        }
        else {
            prev->right = node;
        }
        
    }
    
    // Map the memory region into the l2_pagetable in chunks
    for (size_t i = 0; i < bytes; i += BASE_PAGE_SIZE) {
        
        // Allocate a new node for the new mapping
        struct pt_cap_tree_node *map_node = slab_alloc(&current.slabs);;
        
        // Allocate a new slot for the mapping capability
        errval_t err_slot_alloc = slot_alloc(&map_node->mapping_cap);
        if (!err_is_ok(err_slot_alloc)) {
            return err_slot_alloc;
        }
        
        // Map the frame into the appropriate slot in the L2 pagetable
        errval_t err_frame_map = vnode_map(node->cap, frame, l2_offset, flags, i, 1, map_node->mapping_cap);
        if (!err_is_ok(err_frame_map)) {
            slot_free(map_node->mapping_cap);
            return err_frame_map;
        }
        
        // Store the frame capability and L2 page table offset
        map_node->cap = frame;
        map_node->offset = mapping_offset;
        
        // Store the new node in the mapping capability tree
        if (st->mapping_tree_root == NULL) {
            st->mapping_tree_root = map_node;
        } else {
            struct pt_cap_tree_node *prev_map = st->mapping_tree_root;
            while (prev_map != NULL) {
                if (mapping_offset < prev_map->offset) {
                    if (prev_map->left != NULL) {
                        prev_map = prev_map->left;
                    } else {
                        prev_map->left = map_node;
                        break;
                    }
                }
                else if (l1_offset > prev_map->offset) {
                    if (prev_map->right != NULL) {
                        prev_map = prev_map->right;
                    } else {
                        prev_map->right = map_node;
                        break;
                    }
                }
            }
        }
        
        
    }
    
    return SYS_ERR_OK;
}

/**
 * \brief unmap a user provided frame, and return the VA of the mapped
 *        frame in `buf`.
 * NOTE: Implementing this function is optional.
 */
errval_t paging_unmap(struct paging_state *st, const void *region)
{
    return SYS_ERR_OK;
}
