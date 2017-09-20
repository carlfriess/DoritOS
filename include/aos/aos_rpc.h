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

struct aos_rpc {
    // TODO: add state for your implementation
};

/**
 * \brief send a string over the given channel
 */
errval_t aos_rpc_send_string(struct aos_rpc *chan, const char *string);

/**
 * \brief request a RAM capability with >= request_bits of size over the given
 * channel.
 */
errval_t aos_rpc_get_ram_cap(struct aos_rpc *chan, size_t request_bits,
                             struct capref *retcap, size_t *ret_bits);

/**
 * \brief get one character from the serial port
 */
errval_t aos_rpc_serial_getchar(struct aos_rpc *chan, char *retc);

/**
 * \brief send one character to the serial port
 */
errval_t aos_rpc_serial_putchar(struct aos_rpc *chan, char c);

/**
 * \brief Request device memory capability from memory server.
 */
errval_t aos_rpc_get_dev_cap(struct aos_rpc *chan, lpaddr_t paddr,
                             size_t length, struct capref *retcap,
                             size_t *retlen);

/**
 * \brief Request process manager to start a new process
 * \arg name the name of the process that needs to be spawned (without a
 *           path prefix)
 * \arg newpid the process id of the newly spawned process
 */
errval_t aos_rpc_process_spawn(struct aos_rpc *chan, char *name,
                               coreid_t core, domainid_t *newpid);

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
 * \brief open a file
 * \arg path the file's path
 * \arg fd a numeric identifier (file descriptor) which can then be used to
 * access the open file
 */
errval_t aos_rpc_open(struct aos_rpc *chan, char *path, int *fd);

#define MAXNAMELEN 255
struct aos_dirent {
    /// name of the directory entry
    char name[MAXNAMELEN];
    /// size of the item referenced
    size_t size;
    /// Optional: add more information about a directory entry here
};

/**
 * \brief list a directory's contents
 * \arg path the directory's path
 * \arg dir An array containing a struct dirent for each directory entry. Will
 * be allocated by the rpc implementation. Freeing is the caller's
 * responsibility.
 * \arg elem_count the size of `dir' in elements
 */
errval_t aos_rpc_readdir(struct aos_rpc *chan, char* path,
                         struct aos_dirent **dir, size_t *elem_count);

/**
 * \brief read from an open file
 * \arg fd the file descriptor returned by a previous call to open
 * \arg position the position in the file to read from
 * \arg size the number of bytes to read from the file
 * \arg buf A buffer containing the bytes read. Will
 * be allocated by the rpc implementation. Freeing is the caller's
 * responsibility.
 * \arg buflen the amount of bytes read and stored in `buf'
 */
errval_t aos_rpc_read(struct aos_rpc *chan, int fd, size_t position, size_t size,
                      void** buf, size_t *buflen);

/**
 * \brief close an open file
 * \arg fd the file descriptor returned by a previous call to open
 */
errval_t aos_rpc_close(struct aos_rpc *chan, int fd);

/// Optional modification support for the file system

/**
 * \brief write to an open file
 * \arg fd the file descriptor returned by a previous call to open
 * \arg position the position in the file to write to
 * \arg size the number of bytes actually written to the file
 * \arg buf A buffer containing the bytes to write.
 * \arg buflen the amount of bytes in `buf' that should be written to the file
 */
errval_t aos_rpc_write(struct aos_rpc *chan, int fd, size_t position, size_t *size,
                       void *buf, size_t buflen);

/**
 * \brief create a new file. Will perform an implicit open when successful.
 * Shouldn't succeed when a file (or directory) with the same path exists. Can
 * create missing directories.
 * \arg path the path of the file to create
 * \arg fd the file descriptor of the newly created file
 */
errval_t aos_rpc_create(struct aos_rpc *chan, char *path, int *fd);

/**
 * \brief delete a file. Should fail / be deferred if that file is still open
 * (i.e. there is a fd referencing that file)
 * \arg path the file to delete
 */
errval_t aos_rpc_delete(struct aos_rpc *chan, char *path);

/**
 * \brief Initialize given rpc channel.
 * TODO: you may want to change the inteface of your init function
 */
errval_t aos_rpc_init(struct aos_rpc *rpc);

#endif // _LIB_BARRELFISH_AOS_MESSAGES_H
