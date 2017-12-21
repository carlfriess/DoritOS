/**
 * \file
 * \brief Interface declaration for AOS rpc-like messaging
 */

/*
 * Copyright (c) 2013, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef _LIB_BARRELFISH_AOS_MESSAGES_H
#define _LIB_BARRELFISH_AOS_MESSAGES_H

#include <aos/aos.h>

#define URPC_MessageType_TerminalWrite              URPC_MessageType_User0
#define URPC_MessageType_TerminalWriteUnlock        URPC_MessageType_User1
#define URPC_MessageType_TerminalRead               URPC_MessageType_User2
#define URPC_MessageType_TerminalReadUnlock         URPC_MessageType_User3

#define LMP_MessageType_ProcessDeregister          URPC_MessageType_User0
#define LMP_MessageType_ProcessDeregisterNotify    URPC_MessageType_User0

struct terminal_msg {
    errval_t err;
    bool lock;
    char c;
};

struct aos_rpc {
    // TODO: add state for your implementation
    struct lmp_chan *lc;
    struct waitset mem_ws;  // Dedicated waitset for memory requests
    struct urpc_chan *uc;
};

size_t aos_rpc_terminal_write(const char* buf, size_t len);
size_t aos_rpc_terminal_read(char *buf, size_t len);

/**
 * \brief send a number over the given channel
 */
errval_t aos_rpc_send_number(struct aos_rpc *chan, uintptr_t val);

/**
 * \brief send a string over the given channel
 */
errval_t aos_rpc_send_string(struct aos_rpc *chan, const char *string);

/**
 * \brief request a RAM capability with >= request_bits of size over the given
 * channel.
 */
errval_t aos_rpc_get_ram_cap(struct aos_rpc *chan, size_t bytes, size_t align,
                             struct capref *retcap, size_t *ret_bytes);

/**
 * \brief get one character from the serial port
 */
errval_t aos_rpc_serial_getchar(struct aos_rpc *chan, char *retc);

/**
 * \brief send one character to the serial port
 */
errval_t aos_rpc_serial_putchar(struct aos_rpc *chan, char c);

/**
 * \brief Request process manager to start a new process
 * \arg name the name of the process that needs to be spawned (without a
 *           path prefix)
 * \arg newpid the process id of the newly spawned process
 */
errval_t aos_rpc_process_spawn(struct aos_rpc *chan, char *name,
                               coreid_t core, domainid_t *newpid);

/**
 * \brief Request process manager to start a new process
 * \arg name the name of the process that needs to be spawned (without a
 *           path prefix)
 * \arg terminal_pid the process id of the terminal process to be used by the
                     new process
 * \arg newpid the process id of the newly spawned process
 */
errval_t aos_rpc_process_spawn_with_terminal(struct aos_rpc *chan,
                                             char *name,
                                             coreid_t core,
                                             domainid_t terminal_pid,
                                             domainid_t *newpid);

/**
 * \brief Get name of process with id pid.
 * \arg pid the process id to lookup
 * \arg name A null-terminated character array with the name of the process
 * that is allocated by the rpc implementation. Freeing is the caller's
 * responsibility.
 */
errval_t aos_rpc_process_get_name(struct aos_rpc *chan, domainid_t pid,
                                  char **name);

/**
 * \brief Get process ids of all running processes
 * \arg pids An array containing the process ids of all currently active
 * processes. Will be allocated by the rpc implementation. Freeing is the
 * caller's  responsibility.
 * \arg pid_count The number of entries in `pids' if the call was successful
 */
errval_t aos_rpc_process_get_all_pids(struct aos_rpc *chan,
                                      domainid_t **pids, size_t *pid_count);

/**
 * \brief Returns the PID for the process with the given name.
 */
errval_t aos_rpc_process_get_pid_by_name(const char *name, domainid_t *pid);

/**
 * \brief Gets a capability to device registers
 * \param chan  the rpc channel
 * \param paddr physical address of the device
 * \param bytes number of bytes of the device memory
 * \param frame returned devframe
 */
errval_t aos_rpc_get_device_cap(struct aos_rpc *chan, lpaddr_t paddr, size_t bytes,
                                struct capref *frame);

/**
 * \brief Deregister process with init. Will not return;
 */
errval_t aos_rpc_process_deregister(void);

/**
 * \brief Blocks until given process deregisters
 * \param pid PID of process to wait on
 */
errval_t aos_rpc_process_deregister_notify(domainid_t pid);

/**
 * \brief Initialize given rpc channel.
 * TODO: you may want to change the inteface of your init function, depending
 * on how you design your message passing code.
 */
errval_t aos_rpc_init(struct aos_rpc *rpc, struct lmp_chan *lc);


/**
 * \brief Returns the RPC channel to init.
 */
struct aos_rpc *aos_rpc_get_init_channel(void);

/**
 * \brief Returns the channel to the memory server
 */
struct aos_rpc *aos_rpc_get_memory_channel(void);

/**
 * \brief Returns the channel to the process manager
 */
struct aos_rpc *aos_rpc_get_process_channel(void);

/**
 * \brief Returns the channel to the serial console
 */
struct aos_rpc *aos_rpc_get_serial_channel(void);

#endif // _LIB_BARRELFISH_AOS_MESSAGES_H
