/**
 * \file
 * \brief A library for managing physical memory (i.e., caps)
 */

#include <mm/mm.h>
#include <aos/debug.h>


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
    
    debug_printf("Adding memory at %llx of size %zuMB\n", base, size/1024/1024);

    // Allocating new block for mmnode:
    struct mmnode *newNode = slab_alloc((struct slab_allocator *)&mm->slabs);
    
    newNode->type = NodeType_Free;
    newNode->cap.cap = cap;
    newNode->cap.base = base;
    newNode->cap.size = size;
    newNode->prev = NULL;
    newNode->next = mm->head;
    newNode->parent = newNode;
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
    assert(alignment != 0);
    
    // TODO: What if region is not alligned correctly? Padding?
    
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
        if (node->type == NodeType_Free &&
            node->size >= realSize &&       // TODO: Maybe support using nodes with smaller size than realSize as long as node->size >= size?
            !(node->base % alignment)) {
            break;
        }
        
    }
    
    // Check we actually found a node and then split the memory region
    if (node != NULL) {
        
        // Check if the memory region actually needs to be split
        if (node->size == realSize) {
            node->type = NodeType_Allocated;    // Mark the node as allocated
            *retcap = node->cap.cap;            // Return the capability
            debug_printf("Allocated %zu bytes at %llx with alignment %zu\n", size, node->base, alignment);
            return SYS_ERR_OK;
        }
        
        // Allocate a new slots for the new capabilities
        struct capref newSlot1, newSlot2;
        // TODO: Create slot allocator wrapper
        // TODO: Better error handeling:
        assert(err_is_ok( mm->slot_alloc(mm->slot_alloc_inst, 1, &newSlot1) ));
        assert(err_is_ok( mm->slot_alloc(mm->slot_alloc_inst, 1, &newSlot2) ));
        
        // Allocate new mmnodes
        struct mmnode *newNode1 = slab_alloc((struct slab_allocator *)&mm->slabs);
        struct mmnode *newNode2 = slab_alloc((struct slab_allocator *)&mm->slabs);
        
        // Calculate new bases and sizes
        newNode1->base = node->base;
        newNode1->size = realSize;
        newNode2->base = node->base + realSize;
        newNode2->size = node->size - realSize;
        
        // Set types
        newNode1->type = NodeType_Allocated;
        newNode2->type = NodeType_Free;
        
        // Allocate slots for new capabilities
        // TODO: Handle refill
        assert(err_is_ok( mm->slot_alloc(mm->slot_alloc_inst, 1, &newNode1->cap.cap) ));
        assert(err_is_ok( mm->slot_alloc(mm->slot_alloc_inst, 1, &newNode2->cap.cap) ));
    
        printf("parent base: %llu parent size: %llu\n", node->parent->base, node->parent->size);
        
        
        // Create capabilites for new nodes
        errval_t err1 = cap_retype(newNode1->cap.cap,
                                   node->parent->cap.cap,
                                   newNode1->base - node->parent->base,
                                   mm->objtype,
                                   newNode1->size,
                                   1);
        errval_t err2 = cap_retype(newNode2->cap.cap,
                                   node->parent->cap.cap,
                                   newNode2->base - node->parent->base,
                                   mm->objtype,
                                   newNode2->size,
                                   1);
        
        debug_printf("Retype 1: %s\n", err_getstring(err1));
        debug_printf("Retype 2: %s\n", err_getstring(err2));

        // Make sure we can continue
        assert(err_is_ok(err1));
        assert(err_is_ok(err2));
        
        // Update capability info
        newNode1->cap.base = newNode1->base;
        newNode1->cap.size = newNode1->size;
        newNode2->cap.base = newNode1->base;
        newNode2->cap.size = newNode1->size;
        
        // Link stuff up
        newNode1->parent = node->parent;
        newNode2->parent = node->parent;
        newNode1->prev = node->prev;
        newNode1->next = newNode2;
        newNode2->prev = newNode1;
        newNode2->next = node->next;
        if (node->prev != NULL) {
            node->prev->next = newNode1;
        }
        if (node->next != NULL) {
            node->next->prev = newNode2;
        }
        
        // Alter the head pointer if this is the first mmnode in the list
        if (mm->head == node) {
            mm->head = newNode1;
        }
        
        // Delete this mmnode if it isn't the parent of the region
        if (node->parent != node) {
            // Delete the capability
            cap_delete(node->cap.cap);
            // TODO: Free the slot of the capability
            // Free this node
            slab_free((struct slab_allocator *)&mm->slabs, node);
        }
        
        // Return the allocated capability
        *retcap = newNode1->cap.cap;
        
        debug_printf("Allocated %zu bytes at %llx with alignment %zu\n", realSize, newNode1->base, alignment);
        
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
    // TODO: Implement
    return LIB_ERR_NOT_IMPLEMENTED;
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
    struct mmnode *node;

    // Iterate to desired mmnode
    for(node = mm->head; node != NULL && (node->base != base && node->size != size); node = node->next);

    // Check if node NULL or Free
    assert(node != NULL);
    if (node->type == NodeType_Free) {
        return MM_ERR_NOT_FOUND;
    }

    // Absorb the previous node if free and has same parent
    if (node->prev != NULL && node->prev->type == NodeType_Free && node->parent == node->prev->parent) {
        debug_printf("Absorbing previous node!\n");
        struct mmnode *prev = node->prev;

        // Update base and size
        node->base = prev->base;
        node->size += prev->size;

        // Relink list
        if(node->prev->prev != NULL){
            prev->prev->next = node;
        }
        node->prev = node->prev->prev;

        // Free deallocated node
        slab_free(&mm->slabs, (void *)prev);
    }

    // Absorb the next node if free and has same parent
    if (node->next != NULL && node->next->type == NodeType_Free && node->parent == node->next->parent) {
        debug_printf("Absorbing next node!\n");
        struct mmnode *next = node->next;

        // Update size
        node->size += node->next->size;

        // Relink list
        if(node->next->next != NULL){
            node->next->next->prev = node;
        }
        node->next = node->next->next;

        // Free deallocated
        slab_free(&mm->slabs, (void *)next);
    }

    // TODO: Free Slot and Capability
    node->type = NodeType_Free;
    cap_delete(node->cap.cap);
    debug_printf("mmnode successfully freed!\n");

    return SYS_ERR_OK;
}
