/**
 * \file
 * \brief init process for child spawning
 */

/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/aos_rpc.h>
#include <aos/waitset.h>
#include <aos/paging.h>

static struct aos_rpc *init_rpc, *mem_rpc;

const char *str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  "sed do eiusmod tempor incididunt ut labore et dolore magna "
                  "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
                  "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
                  "Duis aute irure dolor in reprehenderit in voluptate velit "
                  "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
                  "occaecat cupidatat non proident, sunt in culpa qui officia "
                  "deserunt mollit anim id est laborum.";

static errval_t request_and_map_memory(void)
{
    errval_t err;

    size_t bytes;
    struct frame_identity id;
    debug_printf("testing memory server...\n");

    struct paging_state *pstate = get_current_paging_state();

    debug_printf("obtaining cap of %" PRIu32 " bytes...\n", BASE_PAGE_SIZE);

    struct capref cap1;
    err = aos_rpc_get_ram_cap(mem_rpc, BASE_PAGE_SIZE, BASE_PAGE_SIZE, &cap1, &bytes);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not get BASE_PAGE_SIZE cap\n");
        return err;
    }

    struct capref cap1_frame;
    err = slot_alloc(&cap1_frame);
    assert(err_is_ok(err));

    err = cap_retype(cap1_frame, cap1, 0, ObjType_Frame, BASE_PAGE_SIZE, 1);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not retype RAM cap to frame cap\n");
        return err;
    }

    err = invoke_frame_identify(cap1_frame, &id);
    assert(err_is_ok(err));

    void *buf1;
    err = paging_map_frame(pstate, &buf1, BASE_PAGE_SIZE, cap1_frame, NULL, NULL);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not get BASE_PAGE_SIZE cap\n");
        return err;
    }

    debug_printf("got frame: 0x%" PRIxGENPADDR " mapped at %p\n", id.base, buf1);

    debug_printf("performing memset.\n");
    memset(buf1, 0x00, BASE_PAGE_SIZE);



    debug_printf("obtaining cap of %" PRIu32 " bytes using frame alloc...\n",
                 LARGE_PAGE_SIZE);

    struct capref cap2;
    err = frame_alloc(&cap2, LARGE_PAGE_SIZE, &bytes);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not get BASE_PAGE_SIZE cap\n");
        return err;
    }

    err = invoke_frame_identify(cap2, &id);
    assert(err_is_ok(err));

    void *buf2;
    err = paging_map_frame(pstate, &buf2, LARGE_PAGE_SIZE, cap2, NULL, NULL);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not get BASE_PAGE_SIZE cap\n");
        return err;
    }

    debug_printf("got frame: 0x%" PRIxGENPADDR " mapped at %p\n", id.base, buf1);

    debug_printf("performing memset.\n");
    memset(buf2, 0x00, LARGE_PAGE_SIZE);

    return SYS_ERR_OK;

}

static errval_t test_basic_rpc(void)
{
    errval_t err;

    debug_printf("RPC: testing basic RPCs...\n");

    debug_printf("RPC: sending number...\n");
    err =  aos_rpc_send_number(init_rpc, 42);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not send a string\n");
        return err;
    }

    debug_printf("RPC: sending small string...\n");
    err =  aos_rpc_send_string(init_rpc, "Hello init");
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not send a string\n");
        return err;
    }

    debug_printf("RPC: sending large string...\n");
    err =  aos_rpc_send_string(init_rpc, str);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "could not send a string\n");
        return err;
    }

    debug_printf("RPC: testing basic RPCs. SUCCESS\n");

    return SYS_ERR_OK;
}

static void recurse(int i){
    volatile uint32_t buf[10];

    if (i % 10 == 0 || i > 300) {
        debug_printf("CTR: %d, %p\n", i, buf);
    }
    for (int j = 0; j < 10; j++) {
        printf("%d", buf[j]);
    }
//    __asm__ volatile ("sub sp,#16\n\tpush {r3}\n\t");
//    recurse(++i);
}

int main(int argc, char *argv[])
{
    errval_t err;

    /* Test Terminal Get/Put
    char c;
    size_t size;

    do {
        size = aos_rpc_terminal_read(&c, sizeof(char));
        aos_rpc_terminal_write(&c, sizeof(char));
    } while(true);
    */

    debug_printf("memeater started....\n");

    init_rpc = aos_rpc_get_init_channel();
    if (!init_rpc) {
        USER_PANIC("init RPC channel NULL?\n");
    }

    mem_rpc = aos_rpc_get_memory_channel();
    if (!mem_rpc) {
        USER_PANIC("memory RPC channel NULL?\n");
    }

    char *ptr = (char *) malloc(100*1024*1024);
    debug_printf("MALLOC: %p\n", ptr);
    ptr += 50*1024*1024;
    *ptr = 'S';
    debug_printf("CHARACTER: %c\n", *ptr);

    for (int i = 0; i < 512; i++) {
        struct capref cap;
        debug_printf("%d\n", i);
        slot_alloc(&cap);
    }

    recurse(0);
    
    domainid_t pid;
    aos_rpc_process_spawn(init_rpc, "hello", 0, &pid);
    
    char *string;
    aos_rpc_process_get_name(init_rpc, 1, &string);
    debug_printf("Got process name by RPC: %s\n", string);
    
    debug_printf("Getting list of PIDs:\n");
    domainid_t *pids;
    size_t count;
    aos_rpc_process_get_all_pids(init_rpc, &pids, &count);
    for (size_t i = 0; i < count; i++) {
        debug_printf("%d\n", pids[i]);
    }
    
    err = test_basic_rpc();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "failure in testing basic RPC\n");
    }

    err = request_and_map_memory();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "could not request and map memory\n");
    }


    /* test printf functionality */
    debug_printf("testing terminal printf function...\n");

    printf("Hello world using terminal service\n");

    debug_printf("memeater terminated....\n");

    return EXIT_SUCCESS;
}
