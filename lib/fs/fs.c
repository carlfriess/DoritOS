/**
 * \file fs.c
 * \brief Filesystem support library
 */

#include <aos/aos.h>
#include <fs/fs.h>
#include <fs/dirent.h>

#include <fs/vfs.h>

#include <fs/ramfs.h>
#include <fs/fs_rpc.h>

#include "fs_internal.h"

/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetsstrasse 6, CH-8092 Zurich. Attn: Systems Group.
 */

extern void *vfs_state;

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
    
    // TODO!!!
    
    // Bind with fileservice
    
    // Allocate 'mounting' datastructure
    
    ramfs_mount_t ram_mount = NULL;
    err = ramfs_mount("/", &ram_mount);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    
    void *fat_mount = NULL;
    
    // fatfs_mount("/sdcard", &st);
    err = fs_rpc_init(fat_mount);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    /* register libc fopen/fread and friends */
    //fs_libc_init(state);

    // Allocate VFS mount structure to be saved in fopen.c when calling fs_libc_init()
    struct vfs_mount *vfs_st = calloc(1, sizeof(struct vfs_mount));
    
    // Set ram and fat mounts
    vfs_st->ram_mount = ram_mount;
    vfs_st->fat_mount = NULL;
    
    // Allocate mount linked list node with ramfs type and name "/"
    struct mount_node *head = calloc(1, sizeof(struct mount_node));
    head->name = "/";
    head->len = 1;
    head->type = RAMFS;
    head->next = NULL;
    
    // Set head of mount linked list to be ramfs
    vfs_st->head = head;
    
    // Initialize file system libc functions with VFS mount structure as state
    fs_libc_init((void *) vfs_st);
    
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
 * uri: service-name://fstype/params
 */
errval_t filesystem_mount(const char *path, const char *uri)
{
    
    // VFS mount state to get mount linked list
    struct vfs_mount *vfs_mount = vfs_state;
    
    // Set indirect pointer to head of mount linked list
    struct mount_node **indirect = &vfs_mount->head;
    
    // Construct new mount node
    struct mount_node *new_node = calloc(1, sizeof(struct mount_node));
    
    // Set name of new mount node
    new_node->name = strdup(path);
    
    // Set length of new mount node
    new_node->len = strlen(path);
    
    // Try to parse uri service string with given format "service-name://fstype/params"
    char *ptr = strchr(uri, ':');
    if (ptr == NULL) {
        debug_printf("Invalid request\n");
        free(new_node);
        return FAT_ERR_BAD_FS;
    }
    
    char *service = calloc(1, ptr - uri + 1);
    memcpy(service, uri, ptr - uri);
    service[ptr - uri] = '\0';
    
    if (0 == strcmp(service, "ramfs")) {
        new_node->type = RAMFS;
    }
    else if (0 == strcmp(service, "mmchs")) {
        new_node->type = FATFS;
    }
    else {
        debug_printf("Couldn't parse path -> default file system: RAMFS\n");
        new_node->type = RAMFS;
    }
    
    // Add the new mount node to linked list
    while (*indirect != NULL) {
        
        // Check case where exact same mount uri already in list
        if ((strcmp(path, (*indirect)->name)) == 0) {
            debug_printf("This uri has already been mounted\n");
            return SYS_ERR_OK;
        }
    
        // Break when uri is bigger than next node's name
        if (strcmp(path, (*indirect)->name) > 0) {
            break;
        }
        
        // Update indirect
        indirect = &(*indirect)->next;
        
    }
    
    // Insert new node between node with lexicographical bigger and smaller node
    new_node->next = *indirect;
    *indirect = new_node;
    
    //fs_rpc_mount(const char *path, const char *uri);
    
    
    // TODO!!!
    
    
    return SYS_ERR_OK;
    
}




