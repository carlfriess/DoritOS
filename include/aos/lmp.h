#ifndef _AOS_LMP_H_
#define _AOS_LMP_H_

#include <aos/aos.h>

/*
 * LMP Request Protocol
 *
 * ==== Number ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Number
 * arg1: uintptr_t Number
 *
 * cap: NULL_CAP
 *
 * ==== StringShort ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_StringShort
 * arg1-8: char[] String
 *
 * cap: NULL_CAP
 *
 * ==== StringLong ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_StringLong
 * arg1: size_t Size of frame
 *
 * cap: Frame containing string
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
 * arg1: coreid_t Core ID
 * arg2-8: char[] Name
 *
 * cap: NULL_CAP
 *
 * ==== NameLookup ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_NameLookup
 * arg1: domainid_t PID of process to be looked up
 *
 * cap: NULL_CAP
 *
 * ==== PidDiscover ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_PidDiscover
 *
 * cap: NULL_CAP
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
 * ==== Number ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Number
 * arg1: uintptr_t Number
 *
 * cap: NULL_CAP
 *
 * ==== StringShort ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_StringShort
 * arg1: errval_t Status code
 * arg2: size_t Received content length
 *
 * cap: NULL_CAP
 *
 * ==== StringLong ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_StringLong
 * arg1: errval_t Status code
 * arg2: size_t Received content length
 *
 * cap: NULL_CAP
 *
 * ==== Register ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Register
 * arg1: errval_t Status code
 *
 * cap: NULL_CAP
 *
 * ==== Memory Alloc ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryAlloc
 * arg1: errval_t Status code
 *
 * cap: RAM capability to allocated memory
 *
 * ==== Memory Free ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_MemoryFree
 * arg1: errval_t Status code
 *
 * cap: NULL_CAP
 *
 * ==== Spawn ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Spawn
 * arg1: errval_t Status code
 * arg2: domainid_t Process ID of new process
 *
 * cap:
 *
 * ==== PidDiscover ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_PidDiscover
 * arg1: size_t Number of PIDs
 *
 * cap: NULL_CAP
 *
 * ==== Terminal ====
 *
 * arg0: enum lmp_request_type RequestType = LMP_RequestType_Terminal
 * arg1: errval_t Status code
 *
 * cap: NULL_CAP
 *
 */

enum lmp_request_type {
    LMP_RequestType_NULL = 0,
    LMP_RequestType_Number,
    LMP_RequestType_StringShort,
    LMP_RequestType_StringLong,
    LMP_RequestType_Register,
    LMP_RequestType_MemoryAlloc,
    LMP_RequestType_MemoryFree,
    LMP_RequestType_Spawn,
    LMP_RequestType_NameLookup,
    LMP_RequestType_PidDiscover,
    LMP_RequestType_Terminal
};

typedef errval_t (*lmp_server_spawn_handler)(char *name, coreid_t coreid, domainid_t *pid);

// Server side
void lmp_server_dispatcher(void *arg);
void lmp_server_register(struct lmp_chan *lc, struct capref cap);
errval_t lmp_server_memory_alloc(struct lmp_chan *lc, size_t bytes, size_t align);
errval_t lmp_server_memory_free(struct lmp_chan *lc, struct capref cap, size_t bytes);
void lmp_server_spawn(struct lmp_chan *lc, uintptr_t *args);
void lmp_server_spawn_register_handler(lmp_server_spawn_handler handler);
void lmp_server_terminal(struct lmp_chan *lc, struct capref cap);

// Client side
void lmp_client_recv(struct lmp_chan *arg, struct capref *cap, struct lmp_recv_msg *msg);
void lmp_client_wait(void *arg);

errval_t lmp_send_string(struct lmp_chan *lc, const char *string);
errval_t lmp_recv_string(struct lmp_chan *lc, char **string);
errval_t lmp_recv_string_from_msg(struct lmp_chan *lc, struct capref cap,
                                  uintptr_t *words, char **string);

typedef errval_t (*ram_free_handler_t)(struct capref, size_t size);

void register_ram_free_handler(ram_free_handler_t ram_free_function);

#endif
