
<center>![](DoritOS Logo.png)</center>

# Advanced Operating Systems

# Report

<center>Group B - Fall 2017 - DoritOS</center>

<center>Carl Friess, Sven Knobloch, Sebastian Winberg</center>
<div style="page-break-after: always;"></div>


## Memory Management

We chose to us a linked list as our fundamental data structure. Each node represents an allocated or free area of RAM. Further more the nodes are stored in the same order as the corresponding regions occur in memory. This allows us to easily and efficiently coalesce adjacent regions.

```c
struct mmnode {
    enum nodetype type;    ///< Type of `this` node.
    struct capinfo cap;    ///< Cap in which this region exists
    struct mmnode *prev;   ///< Previous node in the list.
    struct mmnode *next;   ///< Next node in the list.
    genpaddr_t base;       ///< Base address of this region
    gensize_t size;        ///< Size of this free region in cap
};
```
<center>*A memory region node*</center>

The struct above describes each node in the linked list. The field `type` marks nodes as free or allocated while the `base` and `size` fields define the precise memory region.

In contrast to the other fields, the `cap` field does not directly relate to the memory region described by the node. Instead it is the RAM capability for the entire RAM region initially added to the memory manager. As we will see, this is necessary for the releasing of memory.

To be precise, the `cap` field actually consists of a struct storing not only the capability for the entire regions, but also the information describing the region:

```c
struct capinfo {
    struct capref cap;
    genpaddr_t base;
    gensize_t size;
};
```
<center>*Struct storing a RAM region and it's capability*</center>

The `prev` and `next` fields are just used for traversal. We considered using the `collections/list` library but decided against it as we need very specialised operations on the list based on the data stored in each node. Ultimately it seemed simpler to manually implement the linked list. In hindsight it might have been wise to instead somehow modify the library to better suit our needs, since as it turns out we used linked lists for many similar purposes later on.

### Adding RAM regions to the memory manager

Although in practice not used, the memory manager allows for an arbitrary amount of memory regions to be added to its purview by calling `mm_add(struct mm *mm, struct capref cap, genpaddr_t base, size_t size)`. For each of these regions it receives a RAM capability spanning the entire region.

When adding a region, the memory manager creates a new node, marks it as free and stores the RAM capability and the information relating to the region. The node is then added to the linked list in no particular order. As we will see, it is only necessary for nodes of the same region to be correctly ordered and grouped together.

### Allocating memory

When servicing a request for a RAM capability of a given size, the list is traversed and a node in the list is chosen by first fit. The requested size and alignment are checked to be a multiple of the page size (`BASE_PAGE_SIZE`) and rounded up accordingly. Other sizes and alignments are not useful, since they cannot be mapped anyway.

Once a node is found it is checked for alignment and the necessary padding is calculated to meet the alignment criteria. If the size of the region is still sufficient, it is then split into two nodes. Furthermore, is also split at the back if the remaining memory region is still larger than the requested size. Like this an allocation can cause a node to become up to three nodes.

This splitting will cause some overhead when traversing the linked list. However, it is only rarely necessary to split at the beginning of a RAM region to meet alignment criteria. Additionally, we are eagerly coalescing nodes when releasing memory, which should help keep the overhead low.

Once the final node has been created the initial RAM capability is retyped and returned to the client. The new capability is not stored, since the client must return it when releasing the memory, making this unnecessary.

### Releasing memory

Clients of the memory allocator can return memory capabilities using the `mm_free(struct mm *mm, struct capref cap, genpaddr_t base, gensize_t size)` function. The allocator is then use the `base` argument to find the corresponding node in the linked list. Once found the returned capability is simply deleted, allowing the initial RAM capability to be retyped again for the same region.

Then the next and previous nodes in the list are checked for the possibility of coalescing. We found that unifying coalescing algorithm of both the preceding and subsequent nodes simplified some otherwise complicated case distinctions, leading to this function for coalescing:

```c
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
```
<center>*Algorithm for coalescing RAM region nodes*</center>

Unfortunately, it is not possible to merge RAM capabilities. This is another reason, (beyond simplicity,) why we only store the initial RAM capability for the entire region and then retype it for each allocation.

### Gathering metadata

We added a function (`mm_available(struct mm *mm, gensize_t *available, gensize_t *total)`) to determine the available and total memory managed by the memory manager. This proved very useful for testing for memory leaks and other bugs in the memory manager.

Since the memory manager currently does not keep any such data, the function needs to traverse the entire linked list each time it is called, which is quite inefficient. However, we ended up only using it for debugging and so deemed this implementation acceptable.

### Allocating memory for the nodes

Obviously we also need memory to store each node of the nodes in the linked list. We used a slab allocator for this purpose. However, even the slab allocator needs to request more memory form the memory manager.

We were provided with an implementation of a slab allocator, but added the implementation of `slab_refill_pages(struct slab_allocator *slabs, size_t bytes)`. The refill process is simple. It requests a RAM capability from the memory manager, retypes it to a frame capability and maps it.

Obviously this creates a circular dependancy on the memory manager. We solved this issue by determining that the slab allocator should always have 5 or more free slabs. When this lower bound is crossed during a RAM allocation, the refill is prematurely triggered. This is stored in the state of the memory manager to prevent an infinite refill loop.

### Allocating slots for capabilities

Thanks to the provided `twolevel_slot_alloc` and `single_slot_alloc` slot allocator implementations, the allocation and refilling of slots was not particularly problematic and very little pre-emptive refilling was necessary.

We were also provided with a slot pre-allocator. However, we weren't sure what was intended for and didn't use it. Instead we simply implemented the `slab_refill_no_pagefault(struct slab_allocator *slabs, struct capref frame, size_t minbytes)` method using only the `slab_default_refill()`, which only executes the previously discussed `slab_refill_pages()`.

