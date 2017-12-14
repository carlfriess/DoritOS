/**
 * \file fs.c
 * \brief Filesystem support library
 */

#include <aos/aos.h>
#include <fs/fs.h>
#include <fs/dirent.h>
#include <fs/ramfs.h>

#include "fs_internal.h"

/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */


/**
 * @brief initializes the filesystem library
 *
 * @return SYS_ERR_OK on success
 *         errval on failure
 *
 * NOTE: This has to be called before any access to the files
 */
errval_t filesystem_init(void)
{
    errval_t err;

    /* TODO: Filesystem project: hook up your init code here */

    ramfs_mount_t st = NULL;
    err = ramfs_mount("/", &st);
    if (err_is_fail(err)) {
        return err;
    }

    /* register libc fopen/fread and friends */
    fs_libc_init(st);

    return SYS_ERR_OK;
}

/**
 * @brief mounts the URI at a give path
 *
 * @param path  path to mount the URI
 * @param uri   uri to mount
 *
 * @return SYS_ERR_OK on success
 *         errval on error
 *
 * This mounts the uri at a given, existing path.
 *
 * path: service-name://fstype/params
 */
errval_t filesystem_mount(const char *path, const char *uri)
{
    return LIB_ERR_NOT_IMPLEMENTED;
}
