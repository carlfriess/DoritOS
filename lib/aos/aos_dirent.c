
/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos.h>
#include <aos/aos_dirent.h>

#define FAIL_FN() \
do { \
    debug_printf("***** %s:%s() called. Something is probably wrong! Maybe " \
                 "you forgot to call aos_register_dirops().\n", __FILE__,__FUNCTION__); \
    return LIB_ERR_NOT_IMPLEMENTED; \
} while (0)

static errval_t aos_mkdir_fail(const char *path){ FAIL_FN(); }
static errval_t aos_rmdir_fail(const char *path){ FAIL_FN(); }
static errval_t aos_opendir_fail(const char *path, aos_dirhandle_t *h){ FAIL_FN(); }
static errval_t aos_readdir_fail(aos_dirhandle_t h, char **name) { FAIL_FN(); }
static errval_t aos_closedir_fail(aos_dirhandle_t h) { FAIL_FN(); }

static struct
{
    aos_mkdir_fn_t mkdir;
    aos_rmdir_fn_t rmdir;
    aos_opendir_fn_t opendir;
    aos_readdir_fn_t readdir;
    aos_closedir_fn_t closedir;
}  dir_ops =
{
    .mkdir = aos_mkdir_fail,
    .rmdir = aos_rmdir_fail,
    .opendir = aos_opendir_fail,
    .readdir = aos_readdir_fail,
    .closedir = aos_closedir_fail
};

/**
 * @brief sets the operation handlers for the directory functions
 *
 * @param ops  struct containing function pointers
 */
void aos_register_dirops(aos_mkdir_fn_t mkdir_fn,
                         aos_rmdir_fn_t rmdir_fn,
                         aos_opendir_fn_t opendir_fn,
                         aos_readdir_fn_t readdir_fn,
                         aos_closedir_fn_t closedir_fn)
{
    dir_ops.mkdir    = (mkdir_fn    ? mkdir_fn    : aos_mkdir_fail);
    dir_ops.rmdir    = (rmdir_fn    ? rmdir_fn    : aos_rmdir_fail);
    dir_ops.opendir  = (opendir_fn  ? opendir_fn  : aos_opendir_fail);
    dir_ops.readdir  = (readdir_fn  ? readdir_fn  : aos_readdir_fail);
    dir_ops.closedir = (closedir_fn ? closedir_fn : aos_closedir_fail);
}

/**
 * @brief opens a directory on the
 *
 * @param path          the path to be created
 * @param ret_handle    returned handle to the opendir state
 *
 * @returns SYS_ERR_OK on successful creation
 *          FS_ERR_EXISTS if there is already a directory or file
 *          FS_ERR_NOTFOUND if there is no such path to the parent
 */
errval_t opendir(const char *path, aos_dirhandle_t *ret_handle)
{
    return dir_ops.opendir(path, ret_handle);
}

/**
 * @brief opens a directory on the
 *
 * @param handle    the handle to close
 *
 * @returns SYS_ERR_OK on successful closing of the directory
 *          FS_ERR_INVALID_FH if the filehandle was invalid
 */
errval_t closedir(aos_dirhandle_t handle)
{
    return dir_ops.closedir(handle);
}

/**
 * @brief reads the next directory entry
 *
 * @param handle    the handle to the directory
 * @param name      returned (malloced) name oft he file
 *
 * @returns SYS_ERR_OK on successful reading the next entry
 *          FS_ERR_INVALID_FH if the directory handle  was invalid
 *          FS_ERR_NOTFOUND there is no next file in the directory
 */
errval_t readdir(aos_dirhandle_t handle, char **name)
{
    return dir_ops.readdir(handle, name);
}

/**
 * @brief creates a new directory on the file system
 *
 * @param path  the path to be created
 *
 * @returns SYS_ERR_OK on successful creation
 *          FS_ERR_EXISTS if there is already a directory or file
 *          FS_ERR_NOTFOUND if there is no such path to the parent
 */
errval_t mkdir(const char *path)
{
    return dir_ops.mkdir(path);
}

/**
 * @brief removes a directory from the file system
 *
 * @param path  the path to be removed
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_NOTEMPTY if the directory is not emtpy
 *          FS_ERR_NOTDIR if the path is not a directory
 *          FS_ERR_NOTFOUND if there is no such path
 */
errval_t rmdir(const char *path)
{
    return dir_ops.rmdir(path);
}
