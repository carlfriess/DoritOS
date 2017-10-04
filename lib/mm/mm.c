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
    assert(alignment != 0);
    
    debug_printf("Allocating %zu bytes with allignment %zu\n", size, alignment);
    
    struct mmnode *node;
    
    // Iterate the list of mmnodes until there are no nodes left
    for (node = mm->head; node != NULL; node = node->next) {
        
        // Break if we found a free mmnode with sufficient size and correct alignment
        if (node->type == NodeType_Free &&
            node->size >= size &&
            !(node->base % alignment)) {
            break;
        }
        
    }
    
    // Check we actually found a node
    if (node != NULL) {
        node->type = NodeType_Allocated;    // Mark the node as allocated
        *retcap = node->cap.cap;            // Return the capability
        debug_printf("Allocated %zu bytes at %llx with alignment %zu\n", size, node->base, alignment);
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

    // Absorb previous node if free and same parent
    if (node->prev != NULL && node->prev->type == NodeType_Free && node->parent == node->prev->parent) {
        node->base = node->prev->base;
        node->size += node->prev->size;

        if(node->prev->prev != NULL){
            node->prev->prev->next = node;
        }
    }

    // Absorb next node if free and same parent
    if (node->next != NULL && node->next->type == NodeType_Free && node->parent == node->next->parent) {
        node->size += node->next->size;

        if(node->next->next != NULL){
            node->next->next->prev = node;
        }
    }

    node->type = NodeType_Free;
    //TODO: Free Slot and Capability
//    node->cap = NULL;

    return SYS_ERR_OK;
}
