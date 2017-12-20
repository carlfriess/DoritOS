//
//  vfs.c
//  DoritOS
//

#include <stdio.h>

#include <fs/ramfs.h>
#include <fs/fs_rpc.h>

#include <fs/vfs.h>

enum fs_type find_mount_type(struct mount_node *head, char *path, char **ret_path) {
    
    assert(ret_path != NULL);
    
    // Set temp to head of mount linked list
    struct mount_node *temp = head;
    
    // Set default type to RAMFS
    enum mount_type ret_type = RAMFS;
    *ret_path = path;
    
    while(temp != NULL) {
        
        // Check if prefix of path matches temp's name
        if (strncmp(path, temp->name, temp->len) == 0) {
            
            // Set return mount type
            ret_type = temp->type;
            
            // Set return path to suffix
            *ret_path = path + temp->len;
            
            break;
        }
        
        // Update temp
        temp = temp->NULL;
    }
    
    return ret_type;
    
}


errval_t vfs_open(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_open(mount->ram_mount, rel_path, &vfs_h->handle);
            break;
        case FATFS:
            err = fs_rpc_open(mount->fat_mount, rel_path, &vfs_h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs open unsuccessful\n");
            break;
    }
    
    // Set file system type of VFS handle
    vfs_h->type
    
    // Set return handle
    *rethandle = vfs_h;
    
    return err;
    
}

errval_t vfs_create(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_create(mount->ram_mount, rel_path, &vfs_h->handle);
            break;
        case FATFS:
            err = fs_rpc_cleate(mount->fat_mount, rel_path, &vfs_h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs create unsuccessful\n");
            break;
    }
    
    // Set file system type of VFS handle
    vfs_h->type
    
    // Set return handle
    *rethandle = vfs_h;
    
    return err;
    
}

errval_t vfs_remove(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_remove(mount->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_remove(mount->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs remove unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_read(void *st, vfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_read(mount->ram_mount, vfs_h->handle, buffer, bytes, bytes_read);
            break;
        case FATFS:
            err = fs_rpc_read(mount->fat_mount, vfs_h->handle, buffer, bytes, bytes_read);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_read(mount->ram_mount, vfs_h->handle, buffer, bytes, bytes_written);
            break;
        case FATFS:
            err = fs_rpc_read(mount->fat_mount, vfs_h->handle, buffer, bytes, bytes_written);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_truncate(mount->ram_mount, vfs_h->handle, bytes);
            break;
        case FATFS:
            // TODO: Not implemented yet
            //err = fs_rpc_truncate(mount->fat_mount, vfs_h->handle, bytes);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_tell(mount->ram_mount, vfs_h->handle, pos);
            break;
        case FATFS:
            err = fs_rpc_tell(mount->fat_mount, vfs_h->handle, pos);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_stat(mount->ram_mount, vfs_h->handle, info);
            break;
        case FATFS:
            err = fs_rpc_stat(mount->fat_mount, vfs_h->handle, info);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs stat unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_seek(void *st, vfs_handle_t handle, fs_whence, offset) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_seek(mount->ram_mount, vfs_h->handle, fs_whence, offset);
            break;
        case FATFS:
            err = fs_rpc_seek(mount->fat_mount, vfs_h->handle, fs_whence, offset);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_close(mount->ram_mount, vfs_h->handle);
            break;
        case FATFS:
            err = fs_rpc_close(mount->fat_mount, vfs_h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs close unsuccessful\n");
            break;
    }
    
    // Free VFS handle
    free(vfs_h);
    
    return err;
    
}

errval_t vfs_opendir(void *st, const char *path, vfs_handle_t *rethandle) {
    
    errval_t err;
    
    assert(rethandle != NULL);
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = calloc(1, sizeof(struct vfs_handle));
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_opendir(mount->ram_mount, rel_path, &vfs_h->handle);
            break;
        case FATFS:
            err = fs_rpc_opendir(mount->fat_mount, rel_path, &vfs_h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs opendir unsuccessful\n");
            break;
    }
    
    // Set file system type of VFS handle
    vfs_h->type
    
    // Set return handle
    *rethandle = vfs_h;
    
    return err;
    
}

//errval_t vfs_dir_read_next(void *st, ramfs_handle_t inhandle, char **retname, struct fs_fileinfo *info);
//vfs_readdir(mount, h, name, NULL);
errval_t vfs_dir_read_next(void *st, vfs_handle_t handle, char **retname, struct fs_fileinfo *info) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_dir_read_next(mount->ram_mount, vfs_h->handle, retname, info);
            break;
        case FATFS:
            err = fs_rpc_readdir(mount->fat_mount, vfs_h->handle, retname, info);
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
    struct vfs_mount *mount = st;
    
    // VFS handle to store the FS specific handle and the type
    struct vfs_handle *vfs_h = handle;
    
    switch (vfs_h->type) {
        case RAMFS:
            err = ramfs_closedir(mount->ram_mount, vfs_h->handle);
            break;
        case FATFS:
            err = fs_rpc_closedir(mount->fat_mount, vfs_h->handle);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs close dir unsuccessful\n");
            break;
    }
    
    // Free VFS handle
    free(vfs_h);
    
    return err;
    
}


errval_t vfs_mkdir(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_mkdir(mount->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_mkdir(mount->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs make dir unsuccessful\n");
            break;
    }
    
    return err;
    
}

errval_t vfs_rmdir(void *st, const char *path) {
    
    errval_t err;
    
    // VFS mount state with root directories and mount linked list
    struct vfs_mount *mount = st;
    
    // Mount relative path
    char *rel_path;
    
    // Find the file system type for given path in mount linked list
    enum fs_type type = find_mount_type(mount->head, path, &rel_path);
    
    switch (type) {
        case RAMFS:
            err = ramfs_rmdir(mount->ram_mount, rel_path);
            break;
        case FATFS:
            err = fs_rpc_rmdir(mount->fat_mount, rel_path);
            break;
        default:
            err = SYS_ERR_OK;
            debug_printf("vfs remove dir unsuccessful\n");
            break;
    }
    
    return err;
    
}


//errval_t vfs_mount(const char *uri, vfs_handle_t *retst);
//errval_t ramfs_mount(const char *uri, ramfs_mount_t *retst);




