
/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos.h>
#include <fs/fs.h>
#include <fs/dirent.h>

#define FAIL_FN() \
do { \
    debug_printf("***** %s:%s() called. Something is probably wrong! Maybe " \
                 "you forgot to call fs_register_dirops().\n", __FILE__,__FUNCTION__); \
    return LIB_ERR_NOT_IMPLEMENTED; \
} while (0)

static errval_t fs_mkdir_fail(const char *path){ FAIL_FN(); }
static errval_t fs_rmdir_fail(const char *path){ FAIL_FN(); }
static errval_t fs_rm_fail(const char *path){ FAIL_FN(); }
static errval_t fs_opendir_fail(const char *path, fs_dirhandle_t *h){ FAIL_FN(); }
static errval_t fs_readdir_fail(fs_dirhandle_t h, char **name) { FAIL_FN(); }
static errval_t fs_closedir_fail(fs_dirhandle_t h) { FAIL_FN(); }
static errval_t fs_fstat_fail(fs_dirhandle_t h, struct fs_fileinfo *b) { FAIL_FN(); }

static struct
{
    fs_mkdir_fn_t mkdir;
    fs_rmdir_fn_t rmdir;
    fs_rm_fn_t rm;
    fs_opendir_fn_t opendir;
    fs_readdir_fn_t readdir;
    fs_closedir_fn_t closedir;
    fs_fstat_fn_t fstat;
}  dir_ops =
{
    .mkdir = fs_mkdir_fail,
    .rm = fs_rm_fail,
    .rmdir = fs_rmdir_fail,
    .opendir = fs_opendir_fail,
    .readdir = fs_readdir_fail,
    .closedir = fs_closedir_fail,
    .fstat = fs_fstat_fail
};

/**
 * @brief sets the operation handlers for the directory functions
 *
 * @param ops  struct containing function pointers
 */
void fs_register_dirops(fs_mkdir_fn_t mkdir_fn,
                         fs_rmdir_fn_t rmdir_fn,
                         fs_rm_fn_t rm_fn,
                         fs_opendir_fn_t opendir_fn,
                         fs_readdir_fn_t readdir_fn,
                         fs_closedir_fn_t closedir_fn,
                         fs_fstat_fn_t fstat_fn)
{
    dir_ops.mkdir    = (mkdir_fn    ? mkdir_fn    : fs_mkdir_fail);
    dir_ops.rmdir    = (rmdir_fn    ? rmdir_fn    : fs_rmdir_fail);
    dir_ops.opendir  = (opendir_fn  ? opendir_fn  : fs_opendir_fail);
    dir_ops.readdir  = (readdir_fn  ? readdir_fn  : fs_readdir_fail);
    dir_ops.closedir = (closedir_fn ? closedir_fn : fs_closedir_fail);
    dir_ops.fstat    = (fstat_fn    ? fstat_fn    : fs_fstat_fail);
    dir_ops.rm    = (rm_fn    ? rm_fn    : fs_rm_fail);
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
errval_t opendir(const char *path, fs_dirhandle_t *ret_handle)
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
errval_t closedir(fs_dirhandle_t handle)
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
errval_t readdir(fs_dirhandle_t handle, char **name)
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


/**
 * @brief removes a file from the file system
 *
 * @param path  the path to be removed
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_NOTDIR if the path is not a directory
 *          FS_ERR_NOTFOUND if there is no such path
 */
errval_t rm(const char *path)
{
    return dir_ops.rm(path);
}

/**
 * @brief Obtains file status information
 *
 * @param handle    file handle
 * @param buf       returned data of the filehandle information
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_INVALID_FH if the directory is not emtpy
 */
errval_t fstat(fs_dirhandle_t handle, struct fs_fileinfo *buf)
{
    return dir_ops.fstat(handle, buf);
}

/**
 * @brief Obtains file status information
 *
 * @param handle    file handle
 * @param buf       returned data of the filehandle information
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_NOTFOUND if there is no such path
 */
errval_t stat(const char *path, struct fs_fileinfo *buf)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return FS_ERR_NOTFOUND;
    }

    errval_t err = fstat(f, buf);

    fclose(f);

    return err;
}
