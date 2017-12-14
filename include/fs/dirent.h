
/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef FS_DIRENT_H_
#define FS_DIRENT_H_ 1

#include <fs/fs.h>

typedef void * fs_dirhandle_t;

typedef errval_t (*fs_mkdir_fn_t)(const char *);
typedef errval_t (*fs_rm_fn_t)(const char *);
typedef errval_t (*fs_rmdir_fn_t)(const char *);
typedef errval_t (*fs_opendir_fn_t)(const char *, fs_dirhandle_t *);
typedef errval_t (*fs_readdir_fn_t)(fs_dirhandle_t, char **);
typedef errval_t (*fs_closedir_fn_t)(fs_dirhandle_t);
typedef errval_t (*fs_fstat_fn_t)(fs_dirhandle_t, struct fs_fileinfo*);


/**
 * @brief sets the operation handlers for the directory functions
 *
 * @param ops  struct containing function pointers
 */
void fs_register_dirops(fs_mkdir_fn_t mkdir,
                        fs_rmdir_fn_t rmdir,
                        fs_rm_fn_t rm,
                        fs_opendir_fn_t opendir,
                        fs_readdir_fn_t readdir,
                        fs_closedir_fn_t closedir,
                        fs_fstat_fn_t fstat);

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
errval_t opendir(const char *path, fs_dirhandle_t *ret_handle);

/**
 * @brief opens a directory on the
 *
 * @param handle    the handle to close
 *
 * @returns SYS_ERR_OK on successful closing of the directory
 *          FS_ERR_INVALID_FH if the filehandle was invalid
 */
errval_t closedir(fs_dirhandle_t handle);

/**
 * @brief reads the next directory entry
 *
 * @param handle    the handle to the directory
 *
 * @returns SYS_ERR_OK on successful reading the next entry
 *          FS_ERR_INVALID_FH if the directory handle  was invalid
 *          FS_ERR_NOTFOUND there is no next file in the directory
 */
errval_t readdir(fs_dirhandle_t handle, char **name);

/**
 * @brief creates a new directory on the file system
 *
 * @param path  the path to be created
 *
 * @returns SYS_ERR_OK on successful creation
 *          FS_ERR_EXISTS if there is already a directory or file
 *          FS_ERR_NOTFOUND if there is no such path to the parent
 */
errval_t mkdir(const char *path);

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
errval_t rmdir(const char *path);

/**
 * @brief Obtains file status information
 *
 * @param handle    file handle
 * @param buf       returned data of the filehandle information
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_INVALID_FH if the directory is not emtpy
 */
errval_t fstat(fs_dirhandle_t handle, struct fs_fileinfo *buf);

/**
 * @brief Obtains file status information
 *
 * @param handle    file handle
 * @param buf       returned data of the filehandle information
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_NOTFOUND if there is no such path
 */
errval_t stat(const char *path, struct fs_fileinfo *buf);

/**
 * @brief removes a file from the file system
 *
 * @param path  the path to be removed
 *
 * @returns SYS_ERR_OK on successful removal
 *          FS_ERR_NOTDIR if the path is not a directory
 *          FS_ERR_NOTFOUND if there is no such path
 */
errval_t rm(const char *path);

#endif /* AOS_DIRENT_H_ */
