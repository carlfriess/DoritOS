#ifndef _AOS_LMP_H_
#define _AOS_LMP_H_

#include <aos/aos.h>

/*
 * LMP Request Protocol
 *
 * ==== Register ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Register
 *
 * cap: capability to endpoint of client
 *
 * ==== Memory Alloc ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryAlloc
 * arg1: bytes
 * arg2: align
 *
 * cap: NULL_CAP
 *
 * ==== Memory Free ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryFree
 * arg1: bytes
 *
 * cap: capability to memory to free
 *
 * ==== Spawn ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Spawn
 *
 * ==== Terminal ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Terminal
 *
 * cap: NULL_CAP
 *
 */

/*
 * LMP Response Protocol
 *
 * ==== Register ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Register
 * arg1: errval_t Error
 *
 * cap: NULL_CAP
 *
 * ==== Memory Alloc ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryAlloc
 * arg1: errval_t Error
 *
 * cap: RAM capability to allocated memory
 *
 * ==== Memory Free ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryFree
 * arg1: errval_t Error
 *
 * cap: NULL_CAP
 *
 * ==== Spawn ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Spawn
 * arg1: errval_t Error
 *
 * cap:
 *
 * ==== Terminal ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Terminal
 * arg1: errval_t Error
 *
 * cap: NULL_CAP
 *
 */

enum lmp_request_type {
    LMP_RequestType_NULL = 0,
    LMP_RequestType_Debug,
    LMP_RequestType_Register,
    LMP_RequestType_MemoryAlloc,
    LMP_RequestType_MemoryFree,
    LMP_RequestType_Spawn,
    LMP_RequestType_Terminal
};

// Server side
void lmp_server_dispatcher(void *arg);
void lmp_server_register(struct lmp_chan *lc, struct capref cap);
errval_t lmp_server_memory_alloc(struct lmp_chan *lc, size_t bytes, size_t align);
errval_t lmp_server_memory_free(struct lmp_chan *lc, struct capref cap, size_t bytes);
void lmp_server_spawn(struct lmp_chan *lc, struct capref cap);
void lmp_server_terminal(struct lmp_chan *lc, struct capref cap);

// Client side
void lmp_client_recv(struct lmp_chan *arg, struct capref *cap, struct lmp_recv_msg *msg);
void lmp_client_wait(void *arg);

typedef errval_t (*ram_free_handler_t)(struct capref, size_t size);

void register_ram_free_handler(ram_free_handler_t ram_free_function);

#endif
