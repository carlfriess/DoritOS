/**
 * \file fs.h
 * \brief
 */

#ifndef INCLUDE_FS_FS_H_
#define INCLUDE_FS_FS_H_


#define FS_PATH_SEP '/'

/// Enum defining interpretation of offset argument to #vfs_seek
enum fs_seekpos {
    FS_SEEK_SET,   ///< Offset relative to start of file
    FS_SEEK_CUR,   ///< Offset relative to current position
    FS_SEEK_END,   ///< Offset relative to end of file
};

enum fs_filetype {
    FS_FILE,       ///< Regular file
    FS_DIRECTORY,  ///< Directory
};

/// Data returned from #fs_stat
struct fs_fileinfo {
    enum fs_filetype type;  ///< Type of the object
    size_t size;            ///< Size of the object (in bytes, for a regular file)
};

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
errval_t filesystem_init(void);

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
 */
errval_t filesystem_mount(const char *path, const char *uri);


/*
 * ===========================================================================
 * Below: part of the master solution
 * ===========================================================================
 */

/**
 * @brief acquires a block from the fil
 * @param id    id of the block to read
 * @param data  returns data
 *
 * @return SYS_ERR_OK on success, errval on failure
 */
errval_t filesystem_block_acquire(size_t id, uint8_t *data);

/**
 * @brief writes back a block to disk
 * @param id    id of the block to read
 * @param data  data to write
 *
 * @return SYS_ERR_OK on success, errval on failure
 */
errval_t filesystem_block_write_back(size_t id, uint8_t *data);

#endif /* INCLUDE_FS_FS_H_ */
