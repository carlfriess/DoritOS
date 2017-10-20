/**
 * \file
 * \brief Hello world application
 */

/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, CAB F.78, Universitaetstr. 6, CH-8092 Zurich,
 * Attn: Systems Group.
 */


#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/paging.h>

int main(int argc, char *argv[])
{
    
    printf("Hello, world! from userspace\n");
    
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    
    void *buf;
    paging_alloc(get_current_paging_state(), &buf, BASE_PAGE_SIZE);
    printf("Allocated virtual address space at: %p\n", buf);
    
    return 0;
}
