/**
 * \file
 * \brief Morecore implementation for malloc
 */

/*
 * Copyright (c) 2007 - 2016 ETH Zurich.
 * Copyright (c) 2014, HP Labs.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos.h>
#include <aos/core_state.h>
#include <aos/morecore.h>
#include <stdio.h>

#define PRINT_DEBUG 0

typedef void *(*morecore_alloc_func_t)(size_t bytes, size_t *retbytes);
extern morecore_alloc_func_t sys_morecore_alloc;

typedef void (*morecore_free_func_t)(void *base, size_t bytes);
extern morecore_free_func_t sys_morecore_free;

// this define makes morecore use an implementation that just has a static
// 16MB heap.
//#define USE_STATIC_HEAP


#ifdef USE_STATIC_HEAP

// dummy mini heap (16M)

#define HEAP_SIZE (1<<24)

static char mymem[HEAP_SIZE] = { 0 };
static char *endp = mymem + HEAP_SIZE;

/**
 * \brief Allocate some memory for malloc to use
 *
 * This function will keep trying with smaller and smaller frames till
 * it finds a set of frames that satisfy the requirement. retbytes can
 * be smaller than bytes if we were able to allocate a smaller memory
 * region than requested for.
 */
static void *morecore_alloc(size_t bytes, size_t *retbytes)
{
    // Statically initializing thread mutex
    static struct thread_mutex mutex = { 0, NULL, NULL, 0 };
    
    // Locking thread mutex
    thread_mutex_lock(&mutex);

    struct morecore_state *state = get_morecore_state();

    size_t aligned_bytes = ROUND_UP(bytes, sizeof(Header));
    void *ret = NULL;
    if (state->freep + aligned_bytes < endp) {
        ret = state->freep;
        state->freep += aligned_bytes;
    }
    else {
        aligned_bytes = 0;
    }
    *retbytes = aligned_bytes;

    // Unlock thread mutex
    thread_mutex_unlock(&mutex);

    return ret;
}

static void morecore_free(void *base, size_t bytes)
{
    return;
}

errval_t morecore_init(void)
{
    struct morecore_state *state = get_morecore_state();

    thread_mutex_init(&state->mutex);

    state->freep = mymem;

    sys_morecore_alloc = morecore_alloc;
    sys_morecore_free = morecore_free;
    return SYS_ERR_OK;
}

#else
// dynamic heap using lib/aos/paging features

/**
 * \brief Allocate some memory for malloc to use
 *
 * This function will keep trying with smaller and smaller frames till
 * it finds a set of frames that satisfy the requirement. retbytes can
 * be smaller than bytes if we were able to allocate a smaller memory
 * region than requested for.
 */
static void *morecore_alloc(size_t bytes, size_t *retbytes)
{
    struct morecore_state *state = get_morecore_state();
    
    void *buf;
    paging_region_map(&state->region, bytes, &buf, retbytes);

#if PRINT_DEBUG
    debug_printf("MORECORE_ALLOC: %p\n", buf);
#endif

    return buf;
}

static void morecore_free(void *base, size_t bytes)
{
    USER_PANIC("NYI");
}

errval_t morecore_init(void)
{
    struct morecore_state *state = get_morecore_state();
    
    // Initialize a paging region with 512MB of address space
    paging_region_init(get_current_paging_state(), &state->region, 0x20000000);

#if PRINT_DEBUG
    debug_printf("MORECORE_INIT: %p\n", state->region.base_addr);
#endif

    sys_morecore_alloc = morecore_alloc;
    sys_morecore_free = morecore_free;
    return SYS_ERR_OK;
}

#endif

Header *get_malloc_freep(void);
Header *get_malloc_freep(void)
{
    return get_morecore_state()->header_freep;
}
