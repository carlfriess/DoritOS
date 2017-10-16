/**
 * \file
 * \brief A library for managing physical memory (i.e., caps)
 */

#include <mm/mm.h>
#include <aos/debug.h>

#define PRINT_DEBUG 0

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
    mm->is_refilling = 0;
    
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
    
#if PRINT_DEBUG
    debug_printf("Adding %zu bytes of memory at 0x%llx\n", size, base);
#endif

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
    
    // Disallow alignments and sizes of 0
    assert(alignment != 0);
    assert(size != 0);
    
#if 0
    gensize_t avail, total;
    mm_available(mm, &avail, &total);
    debug_printf("Free memory: %llu\n", avail);
#endif
    
#if PRINT_DEBUG
    debug_printf("Allocating %zu bytes with alignment %zu\n", size, alignment);
#endif
    
    // Make the size is a multiple of the base page size
    if (size % BASE_PAGE_SIZE) {
        size_t tempSize = size / BASE_PAGE_SIZE;
        tempSize++;
        tempSize *= BASE_PAGE_SIZE;
        size = tempSize;
    }
    
    // Check the alignment is a multiple of the base page size
    if (alignment % BASE_PAGE_SIZE) {
        size_t tempAlignment = alignment / BASE_PAGE_SIZE;
        tempAlignment++;
        tempAlignment *= BASE_PAGE_SIZE;
        alignment = tempAlignment;
    }
    
    struct mmnode *node;
    size_t padding = 0;
    
    // Iterate the list of mmnodes
    for (node = mm->head; node != NULL; node = node->next) {

        // Calculate the amount of padding needed at the beginning of the block to match the alignment criteria
        padding = (node->base % alignment != 0) ? alignment - (node->base % alignment) : 0;

        // Break if we found a free mmnode with sufficient size and correct alignment
        if (node->type == NodeType_Free &&
            node->size >= padding &&    // Preventing underflow in next line
            node->size - padding >= size) {
            break;
        }
        
    }

    // Check if we actually found a node
    if (node != NULL) {
        
        // Mark the node as allocated
        node->type = NodeType_Allocated;

        // Check if the memory region needs to be split at the back
        if (node->size != padding + size) {
            
            // Allocate new mmnode that will follow the `node` mmnode
            struct mmnode *newNode = slab_alloc((struct slab_allocator *)&mm->slabs);
            
            // Calculate new bases and sizes
            newNode->base = node->base + padding + size;
            newNode->size = node->size - padding - size;
            node->size = padding + size;
            
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
        
        // Check if the memory region needs to be split at the front
        if (padding != 0) {
            
            // Allocate new mmnode that will precede the `node` mmnode
            struct mmnode *newNode = slab_alloc((struct slab_allocator *)&mm->slabs);
            
            // Calculate new bases and sizes
            newNode->base = node->base;
            newNode->size = padding;
            node->base += padding;
            node->size -= padding;
            
            // Set type of newNode
            newNode->type = NodeType_Free;
            
            // Copy capability info
            newNode->cap = node->cap;
            
            // Link stuff up
            newNode->prev = node->prev;
            newNode->next = node;
            if (node->prev != NULL) {
                node->prev->next = newNode;
            }
            node->prev = newNode;
            if (mm->head == newNode) {
                mm->head = newNode;
            }
        }
        
        // Allocate a new slot for the returned capability
        errval_t errSlot = slot_alloc(retcap);
        if (!err_is_ok(errSlot)) {
            return errSlot;
        }
        
        // Return capability for the allocated region
        errval_t errRetype = cap_retype(*retcap,
                                        node->cap.cap,
                                        node->base - node->cap.base,
                                        mm->objtype,
                                        node->size,
                                        1);
        
        // Make sure we can continue
        if (!err_is_ok(errRetype)) {
            debug_printf("Retype failed: %s\n", err_getstring(errRetype));
            return errRetype;
        }

        // Check that there are sufficient slabs left in the slab allocator
        size_t freecount = slab_freecount((struct slab_allocator *)&mm->slabs);
        if (freecount <= 4 && !mm->is_refilling) {
            mm->is_refilling = 1;
            slab_default_refill((struct slab_allocator *)&mm->slabs);
            mm->is_refilling = 0;
        }
        
        // Summary
#if PRINT_DEBUG
        debug_printf("Allocated %llu bytes at %llx with alignment %zu\n", node->size, node->base, alignment);
#endif
        
        return SYS_ERR_OK;
    }
    
    return MM_ERR_NOT_FOUND;
    
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
    
#if PRINT_DEBUG
    debug_printf("Freeing %llu bytes of memory at 0x%llx\n", size, base);
#endif
    
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
    
    // Delete this capability
    errval_t errDelete = cap_delete(cap);
    if (!err_is_ok(errDelete)) {
        return errDelete;
    }
    
    // Mark the region as free
    node->type = NodeType_Free;

    // Free the slot for the removed node
    slot_free(cap);

    // Absorb the previous node if it is free and has the same parent capability
    if (node->prev != NULL &&
        node->prev->type == NodeType_Free &&
        node->cap.base == node->prev->cap.base &&
        node->cap.size == node->prev->cap.size) {

#if PRINT_DEBUG
        debug_printf("Coalescing with previous node\n");
#endif

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

#if PRINT_DEBUG
        debug_printf("Coalescing with next node\n");
#endif

        // Coalesce with next node
        coalesce_next(mm, node);
    }

    // Summary
#if PRINT_DEBUG
    debug_printf("Done! Free block of %llu bytes at 0x%llx\n", node->size, node->base);
#endif

    return SYS_ERR_OK;
}

void coalesce_next(struct mm *mm, struct mmnode *node) {
    // Sanity checks
    assert(node->next != NULL);
    assert(node->base + node->size == node->next->base);

    // Update size
    node->size += node->next->size;

    struct mmnode *next_node = node->next;

    // Remove the next node from the linked list
    node->next = node->next->next;
    if (node->next != NULL) {
        node->next->prev = node;
    }

    // Free the memory for the removed node
    slab_free(&mm->slabs, next_node);
}

errval_t mm_available(struct mm *mm, gensize_t *available, gensize_t *total) {

    *available = 0;
    *total = 0;
    
    for (struct mmnode *node = mm->head; node != NULL; node = node->next) {
        if (node->type == NodeType_Free) {
            *available += node->size;
        }
        *total += node->size;
    }
    
    return SYS_ERR_OK;
    
}

