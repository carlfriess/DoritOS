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

#define PRINT_DEBUG 0

void insert_free_vspace_node(struct paging_state *st, lvaddr_t base, size_t size);
errval_t free_deletion_node(struct paging_state *st, struct pt_cap_tree_node *node, struct pt_cap_tree_node *l2_node);

static struct paging_state current;

/**
 * \brief Helper function that allocates a slot and
 *        creates a ARM l2 page table capability
 */
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
    // TODO (M4): Implement page fault handler that installs frames when a page fault
    // occurs and keeps track of the virtual address space.
    
    
    // Storing the reference to the slot allocator
    st->slot_alloc = ca;
    
    // Set the capability reference for the l1 page table
    st->l1_pagetable = pdir;
    
    // Set up state for vspace allocation
    st->free_vspace_head = NULL;
    st->free_vspace_base = start_vaddr;

    // Initialize the slab allocator for free vspace nodes
    slab_init(&st->vspace_slabs, sizeof(struct free_vspace_node), slab_default_refill);
    static int first_call = 1;
    if (first_call) {
        static char vspace_nodebuf[sizeof(struct free_vspace_node)*64];
        slab_grow(&st->vspace_slabs, vspace_nodebuf, sizeof(vspace_nodebuf));
    }
    else {
        slab_default_refill(&st->vspace_slabs);
    }
    
    // Initialize the slab allocator for tree nodes
    slab_init(&st->slabs, sizeof(struct pt_cap_tree_node), slab_default_refill);
    if (first_call) {
        static char nodebuf[sizeof(struct pt_cap_tree_node)*64];
        slab_grow(&st->slabs, nodebuf, sizeof(nodebuf));
    }
    else {
        slab_default_refill(&st->slabs);
    }
    
    first_call = 0;
    
    return SYS_ERR_OK;
}

/**
 * \brief This function initializes the paging for this domain
 * It is called once before main.
 */
errval_t paging_init(void)
{
    debug_printf("paging_init\n");
    // TODO (M4): initialize self-paging handler
    // TIP: use thread_set_exception_handler() to setup a page fault handler
    // TIP: Think about the fact that later on, you'll have to make sure that
    // you can handle page faults in any thread of a domain.
    // TIP: it might be a good idea to call paging_init_state() from here to
    // avoid code duplication.
    set_current_paging_state(&current);
    
    // Create the capability reference for the l1 page table at the default location in capability space
    struct capref pdir = {
        .cnode = cnode_page,
        .slot = 0
    };
    
    return paging_init_state(&current,
                             VADDR_OFFSET,
                             pdir,
                             get_default_slot_allocator());
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
    // TIP: you will need to keep track of possible holes in the region
    
    
    
    return SYS_ERR_OK;
}

/**
 *
 * \brief Find a bit of free virtual address space that is large enough to
 *        accomodate a buffer of size `bytes`.
 */
errval_t paging_alloc(struct paging_state *st, void **buf, size_t bytes)
{
    
#if PRINT_DEBUG
    debug_printf("Allocating %zu bytes of virtual address space...\n", bytes);
#endif
    
    // Round up to next page boundary
    if (bytes % BASE_PAGE_SIZE) {
        size_t pages = bytes / BASE_PAGE_SIZE;
        pages++;
        bytes = pages * BASE_PAGE_SIZE;
    }
    
    // Iterate free list and check for suitable address range
    struct free_vspace_node **indirect = &st->free_vspace_head;
    while ((*indirect) != NULL) {
        if ((*indirect)->size >= bytes) {
            break;
        }
        indirect = &(*indirect)->next;
    }
    
    // Check if we found a free address range
    if (*indirect) {
        // Return the base address of the node
        *buf = (void *) (*indirect)->base;
        // Check if free range needs to be split
        if ((*indirect)->size > bytes) {
            // Reconfigure the node
            (*indirect)->base += bytes;
            (*indirect)->size -= bytes;
        }
        else {
            // Remove the node
            *indirect = (*indirect)->next;
            // Free the slab
            slab_free(&st->vspace_slabs, *indirect);
        }
    }
    else {
        // Alocate at the end of the currently managed address range
        *buf = (void *) st->free_vspace_base;
        st->free_vspace_base += bytes;
    }
    
    // Summary
#if PRINT_DEBUG
    debug_printf("Allocated %zu bytes of virtual address space at 0x%x\n", bytes, *buf);
#endif
    
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
    
#if PRINT_DEBUG
    debug_printf("Mapping %d page(s) at 0x%x\n", bytes / BASE_PAGE_SIZE + (bytes % BASE_PAGE_SIZE ? 1 : 0), vaddr);
#endif
    
    for(uintptr_t end_addr, addr = vaddr; addr < vaddr + bytes; addr = end_addr) {
    
        // Find next boundary of L2 page table range
        end_addr = addr / (ARM_L2_MAX_ENTRIES * BASE_PAGE_SIZE);
        end_addr++;
        end_addr *= (ARM_L2_MAX_ENTRIES * BASE_PAGE_SIZE);

        // Calculate size of region to map within this L2 page table
        size_t size = MIN(end_addr - addr, (vaddr + bytes) - addr);


        // Calculate the offsets for the given virtual address
        uintptr_t l1_offset = ARM_L1_OFFSET(addr);
        uintptr_t l2_offset = ARM_L2_OFFSET(addr);
        uintptr_t mapping_offset = addr / BASE_PAGE_SIZE;

        // Search for L2 pagetable capability in the tree
        struct pt_cap_tree_node *node = st->l2_tree_root;
        struct pt_cap_tree_node *prev = node;

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
            node = slab_alloc(&st->slabs);
            node->left = NULL;
            node->right = NULL;

            // Allocate a new slot for the mapping capability
            errval_t err_slot_alloc = st->slot_alloc->alloc(st->slot_alloc, &node->mapping_cap);
            if (!err_is_ok(err_slot_alloc)) {
                return err_slot_alloc;
            }

            // Allocate a new L2 pagetable and get the capability
            errval_t err_l2_alloc = arml2_alloc(st, &node->cap);
            if (!err_is_ok(err_l2_alloc)) {
                slot_free(node->mapping_cap);
                return err_l2_alloc;
            }
            
            // Check for reentrant call of this function
            int skip_l2_creation = 0;
            if (prev && prev->offset > l1_offset && prev->left != NULL) {
                struct pt_cap_tree_node *same_node = prev->left;
                while (same_node != NULL) {
                    // Check if reentrant call created a node for this offset
                    if (l1_offset == same_node->offset) {
                        slab_free(&st->slabs, node);
                        node = same_node;
                        skip_l2_creation = 1;
                        break;
                    }
                    // Move prev to avoid overwriting an existing node
                    prev = same_node;
                    if (l1_offset < same_node->offset) {
                        same_node = same_node->left;
                    }
                    else if (l1_offset > same_node->offset) {
                        same_node = same_node->right;
                    }
                }
            }
            else if (prev && prev->offset < l1_offset && prev->right != NULL) {
                struct pt_cap_tree_node *same_node = prev->right;
                while (same_node != NULL) {
                    // Check if reentrant call created a node for this offset
                    if (l1_offset == same_node->offset) {
                        slab_free(&st->slabs, node);
                        node = same_node;
                        skip_l2_creation = 1;
                        break;
                    }
                    // Move prev to avoid overwriting an existing node
                    prev = same_node;
                    if (l1_offset < same_node->offset) {
                        same_node = same_node->left;
                    }
                    else if (l1_offset > same_node->offset) {
                        same_node = same_node->right;
                    }
                }
            }
            if (!skip_l2_creation) {
                
                // Map L2 pagetable to appropriate slot in L1 pagetable
                errval_t err_l2_map = vnode_map(st->l1_pagetable, node->cap, l1_offset, flags, 0, 1, node->mapping_cap);
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

        }

        // Calculate the number of pages that need to be allocated
        int num_pages = size / BASE_PAGE_SIZE;
        if (size % BASE_PAGE_SIZE) {
            num_pages++;
        }

        // Allocate a new node for the new mapping
        struct pt_cap_tree_node *map_node = slab_alloc(&st->slabs);
        map_node->left = NULL;
        map_node->right = NULL;

        // Allocate a new slot for the mapping capability
        errval_t err_slot_alloc = st->slot_alloc->alloc(st->slot_alloc, &map_node->mapping_cap);
        if (!err_is_ok(err_slot_alloc)) {
            return err_slot_alloc;
        }

        // Map the frame into the appropriate slot in the L2 pagetable
        errval_t err_frame_map = vnode_map(node->cap, frame, l2_offset, flags, addr - vaddr, num_pages, map_node->mapping_cap);
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
                if (mapping_offset == prev_map->offset) {
                    debug_printf("Mapping capability already in mapping tree\n");
                }
                else if (mapping_offset < prev_map->offset) {
                    if (prev_map->left != NULL) {
                        prev_map = prev_map->left;
                    } else {
                        prev_map->left = map_node;
                        break;
                    }
                }
                else if (mapping_offset > prev_map->offset) {
                    if (prev_map->right != NULL) {
                        prev_map = prev_map->right;
                    } else {
                        prev_map->right = map_node;
                        break;
                    }
                }
            }
        }
        
        // Check that there are sufficient slabs left in the slab allocator
        size_t freecount = slab_freecount((struct slab_allocator *)&st->slabs);
        if (freecount <= 6 && !st->slabs_prevent_refill) {
#if PRINT_DEBUG
            debug_printf("Paging slabs allocator refilling...\n");
#endif
            st->slabs_prevent_refill = 1;
            slab_default_refill((struct slab_allocator *)&st->slabs);
            st->slabs_prevent_refill = 0;
        }

    }
    
    // Summary
#if PRINT_DEBUG
    debug_printf("Finished mapping!\n");
#endif
    
    return SYS_ERR_OK;
}

/**
 * \brief unmap a user provided frame, and return the VA of the mapped
 *        frame in `buf`.
 * NOTE: Implementing this function is optional.
 */
errval_t paging_unmap(struct paging_state *st, const void *region)
{
    
    // Setting to be searched key l1_offset
    uintptr_t l1_offset = ARM_L1_OFFSET((lvaddr_t)region);

    // Searching for l2 pagetable capability in the l2 tree
    struct pt_cap_tree_node *l2_node = st->l2_tree_root;
    
    while (l2_node != NULL) {
        
        if (l1_offset < l2_node->offset) {
            l2_node = l2_node->left;
        } else if (l1_offset > l2_node->offset) {
            l2_node = l2_node->right;
        } else {
            break;
        }
        
    }
    
    // Check if l2 tree node was found
    if (l2_node == NULL) {
        debug_printf("l2 pagetable capability not found");
        return -1;
    }

    // Setting to be searched key mapping_offset
    uintptr_t mapping_offset = ((uintptr_t) region) / BASE_PAGE_SIZE;
    
    // Searching for mapping capability in the mapping tree
    struct pt_cap_tree_node **node_indirect = &st->mapping_tree_root;

    while (*node_indirect != NULL) {
        
        if (mapping_offset > (*node_indirect)->offset) {
            node_indirect = &(*node_indirect)->right;
        } else if (mapping_offset < (*node_indirect)->offset) {
            node_indirect = &(*node_indirect)->left;
        } else {
            break;
        }
        
    }
    
    // Check if mapping tree node was found
    if (*node_indirect == NULL) {
        debug_printf("mapping capabilitiy not found");
        return -1;
    }
    
    // Check children of deletion node
    if ((*node_indirect)->left != NULL && (*node_indirect)->right != NULL) {
        
        debug_printf("HAS LEFT AND RIGHT CHILD\n");
        
        // Finding successor to swap with deletion node
        struct pt_cap_tree_node **succ_indirect = node_indirect;
        while((*succ_indirect)->left != NULL) {
            succ_indirect = &(*succ_indirect)->left;
        }
        
        // Relink successor parent with successor child
        struct pt_cap_tree_node *succ = *succ_indirect;
        *succ_indirect = (*succ_indirect)->right;
        
        // Change children of actual successor to have children of deletion node
        succ->left = (*node_indirect)->left;
        succ->right = (*node_indirect)->right;
        
        // Free deletion node and add space to vspace linked list
        free_deletion_node(st, *node_indirect, l2_node);
        *node_indirect = succ;
        
    } else if ((*node_indirect)->left != NULL) {
        
        debug_printf("HAS LEFT CHILD\n");
        
        // Free deletion node and add space to vspace linked list
        free_deletion_node(st, *node_indirect, l2_node);
        *node_indirect = (*node_indirect)->left;
        
    } else if ((*node_indirect)->right != NULL) {
        
        debug_printf("HAS RIGHT CHILD\n");
        
        // Free deletion node and add space to vspace linked list
        free_deletion_node(st, *node_indirect, l2_node);
        *node_indirect = (*node_indirect)->right;
        
    } else {
        
        debug_printf("HAS NO CHILDREN\n");
        
        // Free deletion node and add space to vspace linked list
        free_deletion_node(st, *node_indirect, l2_node);
        *node_indirect = NULL;
        
    }
    
    return SYS_ERR_OK;
}

errval_t free_deletion_node(struct paging_state *st, struct pt_cap_tree_node *node, struct pt_cap_tree_node *l2_node) {
    
    errval_t err;
    
    debug_printf("Freeing slab of deletion node\n");
    
    lvaddr_t base = node->offset;
    size_t size = BASE_PAGE_SIZE;       // FIXME: node can have more than BASE_PAGE_SIZE many bytes?!?!?
    
    debug_printf("Inserting new space in free vspace linked list\n");
    
    // Inserting new space in free vspace linked list
    insert_free_vspace_node(st, base, size);
    
    // Unmapping from l2 pagetable
    err = vnode_unmap(l2_node->cap, node->mapping_cap);
    if (err_is_fail(err)) {
        debug_printf("vnode unmap failed: %s", err_getstring(err));
        return err;
    }
    
    //  TODO: Decide weather to use cap_destroy or cap_delete
    
    // Deleting node mapping capability
    err = cap_delete(node->mapping_cap);
    if (err_is_fail(err)) {
        debug_printf("mapping_cap delete failed: %s", err_getstring(err));
        return err;
    }
    
    // Deleting node capability
    err = cap_delete(node->cap);
    if (err_is_fail(err)) {
        debug_printf("cap delete failed: %s", err_getstring(err));
        return err;
    }
    
    // Freeing tree node slab
    slab_free(&st->slabs, node);
    debug_printf("Freed slab of deletion node\n");
    
    return SYS_ERR_OK;
}

void insert_free_vspace_node(struct paging_state *st, lvaddr_t base, size_t size) {
    
    // Check if list empty or node would have to get new head
    if (st->free_vspace_head == NULL || base + size < st->free_vspace_head->base) {
        
        // Add new_node to head of the linked list
        struct free_vspace_node *new_node = (struct free_vspace_node *) slab_alloc(&st->vspace_slabs);
        new_node->base = base;
        new_node->next = st->free_vspace_head;
        st->free_vspace_head = new_node;
        return;
        
    }
    else if (base + size == st->free_vspace_head->base) {
        
        // Coalescing with front of head
        st->free_vspace_head->base = base;
        st->free_vspace_head->size += size;
        return;
        
    }
    
    // Iterating through loop adding node if necessary
    // Otherwise coalescing by changing base and size of exisiting node
    struct free_vspace_node *node;
    for (node = st->free_vspace_head; node != NULL; node = node->next) {
        
        if (node->next != NULL &&
            node->base + node->size == base &&
            base + size == node->next->base) {
            
            // Coalescing with back of node and front of node->next
            node->size += size + node->next->size;
            // Remove the node->next from the linked list
            struct free_vspace_node *next_node = node->next;
            node->next = node->next->next;
            // Free the memory for the removed node
            slab_free(&st->vspace_slabs, next_node);
            break;
            
        }
        else if (node->next != NULL &&
                 base + size == node->next->base) {
            
            // Coalescing with front of node->next
            node->next->base = base;
            node->next->size += size;
            break;
        }
        else if (node->base + node->size == base) {
            
            // Coalescing with back of node
            node->size += size;
            break;
        
        }
        else if (node->base + node->size < base){
            
            // Add new_node to the linked list between node and node->next (which is potentially null)
            struct free_vspace_node *new_node = (struct free_vspace_node *) slab_alloc(&st->vspace_slabs);
            new_node->base = base;
            new_node->next = node->next;
            node->next = new_node;
            break;
            
        }

    }

    // Update the free_vspace_base in the paging state
    st->free_vspace_base = MAX(st->free_vspace_base, base + size);
    
}

