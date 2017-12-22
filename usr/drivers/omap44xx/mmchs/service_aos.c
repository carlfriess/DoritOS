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

#include <aos/aos.h>

#include "mmchs.h"
#include <fs/fs.h>
#include <fs/dirent.h>
#include <fs/ramfs.h>
//#include <fs/fs_internal.h>

#include <fs_serv/fatfs_rpc_serv.h>
#include <fs_serv/fatfs_serv.h>

__attribute__((unused))
static void print_block(size_t block_nr, void *buf, size_t size) {
    
    char *char_buf = (char *) buf;
    
    debug_printf("Printing block %zu:\n", block_nr);
    
    for (int i = 0; i < size; i += 8) {
        printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
               char_buf[i], char_buf[i+1],char_buf[i+2], char_buf[i+3],
               char_buf[i+4], char_buf[i+5], char_buf[i+6], char_buf[i+7]);
    }
    
}


void init_service(void)
{

    /* initialize the service setup */
    
    errval_t err;
    
    
    // Initialize static variables from BPB region
    err = init_BPB();
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Run the service in a while loop
    run_rpc_serv();

    //USER_PANIC("NYI: Implement RPC service!");
}

