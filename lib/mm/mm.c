/**
 * \file
 * \brief A library for managing physical memory (i.e., caps)
 */

#include <mm/mm.h>
#include <aos/debug.h>

void coalesce_next(struct mm *mm, struct mmnode *node);


/**
 * Initialize the memory manager.
 *
 * \param  mm               The mm struct to initialize.
 * \param  objtype          The cap type this manager should deal with.
 * \param  slab_refill_func Custom function for refilling the slab allocator.
 * \param  slot_alloc_func  Function for allocating capability slots.
 * \param  slot_refill_func Function for refilling (making) new slots.
 * \param  slot_alloc_inst  Pointer to a slot allocator instance (typically passed to the alloc and refill functions).
 */
errval_t mm_init(struct mm *mm, enum objtype objtype,
                 slab_refill_func_t slab_refill_func,
                 slot_alloc_t slot_alloc_func,
                 slot_refill_t slot_refill_func,
                 void *slot_alloc_inst)
{
    assert(mm != NULL);
    
    mm->objtype = objtype;
    mm->slot_alloc = slot_alloc_func;
    mm->slot_refill = slot_refill_func;
    mm->slot_alloc_inst = slot_alloc_inst;
    mm->head = NULL;
    
    // Set the default refill function for the slab allocator
    if (slab_refill_func == NULL) {
        slab_refill_func = slab_default_refill;
    }
    
    slab_init(&mm->slabs, sizeof(struct mmnode), slab_refill_func);
    
    return SYS_ERR_OK;
}

/**
 * Destroys the memory allocator.
 */
void mm_destroy(struct mm *mm)
{
}

/**
 * Adds a capability to the memory manager.
 *
 * \param  cap  Capability to add
 * \param  base Physical base address of the capability
 * \param  size Size of the capability (in bytes)
 */
errval_t mm_add(struct mm *mm, struct capref cap, genpaddr_t base, size_t size)
{
    assert(mm != NULL);
    
    debug_printf("Adding %zu bytes of memory at 0x%llx\n", size, base);

    // Allocating new block for mmnode:
    struct mmnode *newNode = slab_alloc((struct slab_allocator *)&mm->slabs);
    
    newNode->type = NodeType_Free;
    newNode->cap.cap = cap;
    newNode->cap.base = base;
    newNode->cap.size = size;
    newNode->prev = NULL;
    newNode->next = mm->head;
    newNode->base = base;
    newNode->size = size;
    
    if (mm->head != NULL) {
        mm->head->prev = newNode;
    }
    mm->head = newNode;
    
    return SYS_ERR_OK;
}

/**
 * Allocate aligned physical memory.
 *
 * \param       mm        The memory manager.
 * \param       size      How much memory to allocate.
 * \param       alignment The alignment requirement of the base address for your memory.
 * \param[out]  retcap    Capability for the allocated region.
 */
errval_t mm_alloc_aligned(struct mm *mm, size_t size, size_t alignment, struct capref *retcap)
{
    // Disallow an alignment of 0
    assert(alignment != 0);
    
    // Quick fix
    //  TODO: Split beginning if the alignemnt does not match
    if (alignment <= 4096) {
        alignment = 4096;
    }
    
    debug_printf("Allocating %zu bytes with alignment %zu\n", size, alignment);
    
    // Calculate the real size of the region to be allocated, such that it is alligned at the beginning and the end
    size_t realSize = size / alignment;
    if (size % alignment) {
        realSize++;
    }
    realSize *= alignment;
    
    struct mmnode *node;
    
    // Iterate the list of mmnodes until there are no nodes left
    for (node = mm->head; node != NULL; node = node->next) {
        
        // Break if we found a free mmnode with sufficient size and correct alignment
        //  TODO: Maybe support using nodes with smaller size than realSize as long as node->size >= size?
        //  TODO: What if region is not alligned correctly? Padding?
        if (node->type == NodeType_Free &&
            node->size >= realSize &&
            !(node->base % alignment)) {
            break;
        }
        
    }
    
    // Check if we actually found a node
    if (node != NULL) {
        
        // Mark the node as allocated
        node->type = NodeType_Allocated;
        
        // Check if the memory region needs to be split
        if (node->size != realSize) {
            
            // Allocate new mmnode that will follow the `node` mmnode
            struct mmnode *newNode = slab_alloc((struct slab_allocator *)&mm->slabs);
            
            // Calculate new bases and sizes
            newNode->base = node->base + realSize;
            newNode->size = node->size - realSize;
            node->size = realSize;
            
            // Set type of newNode
            newNode->type = NodeType_Free;
            
            // Copy capability info
            newNode->cap = node->cap;
            
            // Link stuff up
            newNode->prev = node;
            newNode->next = node->next;
            if (node->next != NULL) {
                node->next->prev = newNode;
            }
            node->next = newNode;
            
        }
        
        // Allocate a new slot for the returned capability
        //  TODO: Test refill of slots
        assert(err_is_ok( slot_alloc(retcap) ));
        
        // Return capability for the allocated region
        errval_t err = cap_retype(*retcap,
                                  node->cap.cap,
                                  node->base - node->cap.base,
                                  mm->objtype,
                                  node->size,
                                  1);
        
        // Make sure we can continue
        debug_printf("Retype: %s\n", err_getstring(err));
        assert(err_is_ok(err));
        
        // Check that there are sufficient slabs left in the slab allocator
        if (slab_freecount((struct slab_allocator *)&mm->slabs) == 2) {
            slab_default_refill((struct slab_allocator *)&mm->slabs);
        }
    
        // Summary
        debug_printf("Allocated %llu bytes at %llx with alignment %zu\n", node->size, node->base, alignment);
        
        return SYS_ERR_OK;
    }
    
    return -1;
    
}

/**
 * Allocate physical memory.
 *
 * \param       mm        The memory manager.
 * \param       size      How much memory to allocate.
 * \param[out]  retcap    Capability for the allocated region.
 */
errval_t mm_alloc(struct mm *mm, size_t size, struct capref *retcap)
{
    return mm_alloc_aligned(mm, size, BASE_PAGE_SIZE, retcap);
}

/**
 * Free a certain region (for later re-use).
 *
 * \param       mm        The memory manager.
 * \param       cap       The capability to free.
 * \param       base      The physical base address of the region.
 * \param       size      The size of the region.
 */
errval_t mm_free(struct mm *mm, struct capref cap, genpaddr_t base, gensize_t size)
{
    
    debug_printf("Freeing %llu bytes of memory at 0x%llx\n", size, base);
    
    struct mmnode *node;

    // Walk the list of mmnodes until we find the node containing the memory region
    for(node = mm->head; node != NULL; node = node->next) {
        if (node->base == base) {
            break;
        }
    }

    // Check the node was found and isn't allocated
    if (node == NULL || node->type != NodeType_Allocated) {
        debug_printf("Could not find memory region to be freed :(\n");
        return MM_ERR_NOT_FOUND;
    }
    
    // Mark the region as free
    node->type = NodeType_Free;

    // Delete this capability
    assert(err_is_ok( cap_delete(cap) ));

    // Free the slot for the removed node
    slot_free(cap);

    // Absorb the previous node if it is free and has the same parent capability
    if (node->prev != NULL &&
        node->prev->type == NodeType_Free &&
        node->cap.base == node->prev->cap.base &&
        node->cap.size == node->prev->cap.size) {

        debug_printf("Coalescing with previous node\n");

        //Moving to previous node
        node = node->prev;

        // Coalesce with next node
        coalesce_next(mm, node);
    }

    // Absorb the next node if it is free and has the same parent capability
    if (node->next != NULL &&
        node->next->type == NodeType_Free &&
        node->cap.base == node->next->cap.base &&
        node->cap.size == node->next->cap.size) {

        debug_printf("Coalescing with next node\n");

        // Coalesce with next node
        coalesce_next(mm, node);
    }

    // Summary
    debug_printf("Done! Free block of %llu bytes at 0x%llx\n", node->size, node->base);

    return SYS_ERR_OK;
}

void coalesce_next(struct mm *mm, struct mmnode *node) {
    // Sanity checks
    assert(node->next != NULL);
    assert(node->base + node->size == node->next->base);

    // Update size
    node->size += node->next->size;

    struct mmnode* next_node = node->next;

    // Remove the next node from the linked list
    node->next = node->next->next;
    if (node->next != NULL) {
        node->next->prev = node;
    }

    // Free the memory for the removed node
    slab_free(&mm->slabs, next_node);
}
