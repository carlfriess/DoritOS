/*
 * Copyright (c) 2007, 2008, 2009, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef __ASSEMBLER__
#define __ASSEMBLER__
#endif // __ASSEMBLER__

#include <asmoffsets.h> // OFFSETOF etc.
#include <barrelfish_kpi/registers_arch.h> // CPSR_REG etc.
#include <barrelfish_kpi/flags_arch.h> // CPSR_IF_MASK etc.
#include <exceptions.h>
#include <offsets.h>

        .global kernel_stack, kernel_stack_top, exception_vectors

/* This needs to be at the beginning of a 4k frame, that we'll map to the high
 * vectors address. */
.align 12
exception_vectors:
        /* Reset */
        b .
        /* Undefined instruction */
        b .
        /* System call */
        b .
        /* Prefetch abort */
        b .
        /* Data abort */
        b .
        /* Hypervisor trap */
        b .
        /* IRQ */
        b .
        /* FIQ */
        b .

/**
 * \brief Kernel stack.
 *
 * This is the one and only kernel stack for a kernel instance.
 */
        .bss
        .align 8
kernel_stack:
        .space KERNEL_STACK_SIZE, 0
kernel_stack_top:
