//
//  vfs.c
//  DoritOS
//

#include <stdio.h>
#include <stdlib.h>

#include <fs/ramfs.h>
#include <fs/fs_rpc.h>

#include <fs/vfs.h>

enum fs_type find_mount_type(struct mount_node *head, const char *path, char **ret_path) {
    
    assert(ret_path != NULL);
    
    // Set temp to head of mount linked list
    struct mount_node *temp = head;
    
    // Set default type to RAMFS
    enum fs_type ret_type = RAMFS;
    
    while(temp != NULL) {
        
        // Check if prefix of path matches temp's name
        if (strncmp(path, temp->name, temp->len) == 0) {
            
            // Set return mount type
            ret_type = temp->type;
            
            // Set return path to suffix
            *ret_path = strdup(path + temp->len);
            
            return ret_type;
        }
        
        // Update temp
        temp = temp->next;
    }
    
    *ret_path = strdup(path);
    
    return ret_type;
    
}


errval_t vfs_open(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_open(mt->ram_mount, rel_path, &h->handle);
            break;
        case FATFS:
            err = fs_rpc_open(mt->fat_mount, rel_path, &h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs open unsuccessful\n");
            break;
    }
    
    // Free relative path
    free(rel_path);
    
    // Set file system type of VFS handle
    h->type = type;
    
    // Set return handle
    *rethandle = h;
    
    return err;
    
}

errval_t vfs_create(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_create(mt->ram_mount, rel_path, &h->handle);
            break;
        case FATFS:
            err = fs_rpc_create(mt->fat_mount, rel_path, &h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs create unsuccessful\n");
            break;
    }
    
    // Free relative path
    free(rel_path);
    
    // Set file system type of VFS handle
    h->type = type;
    
    // Set return handle
    *rethandle = h;
    
    return err;
    
}

errval_t vfs_remove(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_remove(mt->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_remove(mt->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs remove unsuccessful\n");
            break;
    }
    
    // Free relative path
    free(rel_path);
    
    return err;
    
}

errval_t vfs_read(void *st, vfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_read(mt->ram_mount, h->handle, buffer, bytes, bytes_read);
            break;
        case FATFS:
            err = fs_rpc_read(mt->fat_mount, h->handle, buffer, bytes, bytes_read);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs read unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_write(void *st, vfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_written) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_read(mt->ram_mount, h->handle, buffer, bytes, bytes_written);
            break;
        case FATFS:
            err = fs_rpc_read(mt->fat_mount, h->handle, buffer, bytes, bytes_written);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs write unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_truncate(void *st, vfs_handle_t handle, size_t bytes) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_truncate(mt->ram_mount, h->handle, bytes);
            break;
        case FATFS:
            err = fs_rpc_truncate(mt->fat_mount, h->handle, bytes);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs truncate unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_tell(void *st, vfs_handle_t handle, size_t *pos){
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_tell(mt->ram_mount, h->handle, pos);
            break;
        case FATFS:
            err = fs_rpc_tell(mt->fat_mount, h->handle, pos);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs tell unsuccessful\n");
            break;
    }
    
    return err;
    
}

//errval_t vfs_stat(void *st, vfs_handle_t inhandle, struct fs_fileinfo *info);
errval_t vfs_stat(void *st, vfs_handle_t handle, struct fs_fileinfo *info) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_stat(mt->ram_mount, h->handle, info);
            break;
        case FATFS:
            err = fs_rpc_stat(mt->fat_mount, h->handle, info);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs stat unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_seek(void *st, vfs_handle_t handle, enum fs_seekpos whence, off_t offset) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_seek(mt->ram_mount, h->handle, whence, offset);
            break;
        case FATFS:
            err = fs_rpc_seek(mt->fat_mount, h->handle, whence, offset);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs seek unsuccessful\n");
            break;
    }
    
    return err;
    
}

//errval_t vfs_close(void *st, vfs_handle_t inhandle);
errval_t vfs_close(void *st, vfs_handle_t handle) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_close(mt->ram_mount, h->handle);
            break;
        case FATFS:
            err = fs_rpc_close(mt->fat_mount, h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs close unsuccessful\n");
            break;
    }
    
    // Free VFS handle
    free(h);
    
    return err;
    
}

errval_t vfs_opendir(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_opendir(mt->ram_mount, rel_path, &h->handle);
            break;
        case FATFS:
            err = fs_rpc_opendir(mt->fat_mount, rel_path, &h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs opendir unsuccessful\n");
            break;
    }
    
    // Free relative path
    free((char *) rel_path);
    
    // Set file system type of VFS handle
    h->type = type;
    
    // Set return handle
    *rethandle = h;
    
    return err;
    
}

//errval_t vfs_dir_read_next(void *st, ramfs_handle_t inhandle, char **retname, struct fs_fileinfo *info);
//vfs_readdir(mount, h, name, NULL);
errval_t vfs_dir_read_next(void *st, vfs_handle_t handle, char **retname, struct fs_fileinfo *info) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = handle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_dir_read_next(mt->ram_mount, h->handle, retname, info);
            break;
        case FATFS:
            err = fs_rpc_readdir(mt->fat_mount, h->handle, retname, info);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs read dir unsuccessful\n");
            break;
    }
    
    return err;
    
}


errval_t vfs_closedir(void *st, vfs_handle_t dirhandle) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *h = dirhandle;
    
    switch (h->type) {
        case RAMFS:
            err = ramfs_closedir(mt->ram_mount, h->handle);
            break;
        case FATFS:
            err = fs_rpc_closedir(mt->fat_mount, h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs close dir unsuccessful\n");
            break;
    }
    
    // Free VFS handle
    free(h);
    
    return err;
    
}


errval_t vfs_mkdir(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_mkdir(mt->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_mkdir(mt->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs make dir unsuccessful\n");
            break;
    }
    
    // Free relative path
    free(rel_path);
    
    return err;
    
}

errval_t vfs_rmdir(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mt = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mt->head, path, &rel_path);
    
    debug_printf("path is -%s- -%s- %d\n", path, rel_path, type);
    
    
    
    switch (type) {
        case RAMFS:
            err = ramfs_rmdir(mt->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_rmdir(mt->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs remove dir unsuccessful\n");
            break;
    }
    
    // Free relative path
    free(rel_path);
    
    return err;
    
}


//errval_t vfs_mount(const char *uri, vfs_handle_t *retst);
//errval_t ramfs_mount(const char *uri, ramfs_mount_t *retst);




