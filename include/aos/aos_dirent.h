
/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef AOS_DIRENT_H_
#define AOS_DIRENT_H_ 1

typedef void * aos_dirhandle_t;

typedef errval_t (*aos_mkdir_fn_t)(const char *);
typedef errval_t (*aos_rmdir_fn_t)(const char *);
typedef errval_t (*aos_opendir_fn_t)(const char *, aos_dirhandle_t *);
typedef errval_t (*aos_readdir_fn_t)(aos_dirhandle_t, char **);
typedef errval_t (*aos_closedir_fn_t)(aos_dirhandle_t);


/**
 * @brief sets the operation handlers for the directory functions
 *
 * @param ops  struct containing function pointers
 */
void aos_register_dirops(aos_mkdir_fn_t mkdir,
                         aos_rmdir_fn_t rmdir,
                         aos_opendir_fn_t opendir,
                         aos_readdir_fn_t readdir,
                         aos_closedir_fn_t closedir);

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
errval_t opendir(const char *path, aos_dirhandle_t *ret_handle);

/**
 * @brief opens a directory on the
 *
 * @param handle    the handle to close
 *
 * @returns SYS_ERR_OK on successful closing of the directory
 *          FS_ERR_INVALID_FH if the filehandle was invalid
 */
errval_t closedir(aos_dirhandle_t handle);

/**
 * @brief reads the next directory entry
 *
 * @param handle    the handle to the directory
 *
 * @returns SYS_ERR_OK on successful reading the next entry
 *          FS_ERR_INVALID_FH if the directory handle  was invalid
 *          FS_ERR_NOTFOUND there is no next file in the directory
 */
errval_t readdir(aos_dirhandle_t handle, char **name);

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

#endif /* AOS_DIRENT_H_ */
