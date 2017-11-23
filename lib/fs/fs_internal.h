/*
 * Copyright (c) 2009, 2010, 2011, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef FS_INTERNAL_H_
#define FS_INTERNAL_H_

struct fs_mount;

struct fs_handle {
    void *mount;
};


/*
 * fdtab
 */
#define MIN_FD  0
#ifndef MAX_FD
//#define MAX_FD  132
#define MAX_FD  4096
#endif


enum fdtab_type {
    FDTAB_TYPE_AVAILABLE,
    FDTAB_TYPE_FILE,
    FDTAB_TYPE_UNIX_SOCKET,
    FDTAB_TYPE_STDIN,
    FDTAB_TYPE_STDOUT,
    FDTAB_TYPE_STDERR
};

#include <signal.h>
#include <sys/epoll.h>

struct fdtab_entry {
    enum fdtab_type     type;
//    union {
        void            *handle;
        int             fd;
        int             inherited;
//    };
    int epoll_fd;
};

/* for the newlib glue code */
void fs_libc_init(void *fs_state);

#endif
