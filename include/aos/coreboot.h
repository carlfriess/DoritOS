/**
 * \file
 * \brief User-level routines for core booting.
 */

/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.  If
 * you do not find this file, copies can be found by writing to: ETH Zurich
 * D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <barrelfish_kpi/types.h>

/**
 * \brief Load the relocatable segment of a CPU driver, and relocate it.
 *
 * \param elfdata    Address of the raw ELF image, in memory
 * \param out        Base of the preallocated region, into which the
 *                   relocated segment will be loaded.
 * \param vbase      The kernel-virtual address at which the segment will
 *                   be loaded when executing - this is the physical address
 *                   plus the kernel window offset.
 * \param text_base  The kernel-virtual address of the text segment - this may
 *                   be shared between CPU driver instances.
 * \param got_base   Used to return the kernel-virtual address of the global
 *                   offset table (GOT), once it's been relocated.
 *
 */
errval_t load_cpu_relocatable_segment(void *elfdata, void *out, lvaddr_t vbase,
                                      lvaddr_t text_base, lvaddr_t *got_base);
