/*
 * Copyright (c) 2016, ETH Zurich.
 *
 * This file is distributed under the terms in the attached LICENSE file.  If
 * you do not find this file, copies can be found by writing to: ETH Zurich
 * D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef __KERNEL_CAP_INVOCATIONS
#define __KERNEL_CAP_INVOCATIONS

#include <aos/aos.h>

/**
 * \brief Spawn a new core.
 *
 * \param core_id    APIC ID of the core to try booting
 * \param cpu_type   Barrelfish target CPU type
 * \param entry      Kernel entry point in kernel-virtual memory
 */
static inline errval_t
invoke_monitor_spawn_core(coreid_t core_id, enum cpu_type cpu_type,
                          forvaddr_t entry)
{
    return cap_invoke5(cap_kernel, IPICmd_Send_Start, core_id, cpu_type,
            (uintptr_t)(entry >> 32), (uintptr_t) entry).error;
}

/**
 * \brief Create a capability
 *
 * \param raw        The raw 8-byte capability struct.
 * \param caddr      The CNode into which to create it.
 * \param level      The CNode depth (1 or 2).
 * \param slot       The slot within the CNode
 * \param owner      The owning core
 */
static inline errval_t
invoke_monitor_create_cap(uint64_t *raw, capaddr_t caddr, int level,
                          capaddr_t slot, coreid_t owner)
{
    return cap_invoke6(cap_kernel, KernelCmd_Create_cap, caddr, level, slot,
                       owner, (uintptr_t)raw).error;
}

/**
 * \brief Duplicate ARMv7 core_data into the supplied frame.
 *
 * \param frame    CSpace address of frame capability
 *
 * \return Error code
 */

static inline errval_t
invoke_kcb_clone(struct capref kcb, struct capref frame)
{
    capaddr_t frame_cptr = get_cap_addr(frame);
    enum cnode_type frame_level = get_cap_level(frame);

    return cap_invoke3(kcb, KCBCmd_Clone, frame_cptr, frame_level).error;
}

#endif /* __KERNEL_CAP_INVOCATIONS */
