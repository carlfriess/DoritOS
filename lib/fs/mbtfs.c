//
//  mbtfs.c
//  DoritOS
//
//  Created by Sebastian Winberg on 21.12.17.
//

#include <stdio.h>

#include <fs/fs.h>

#include <fs/mbtfs.h>

struct mbtfs_mount {
    
    char *name;
    
    size_t module_count;
    
    char *modules[];
    
};

struct mbtfs_handle {
    
    char *path;
    bool isdir;
    
    off_t pos;
    
    struct capref frame;
    void *buffer;
    
    size_t size;
    
};

errval_t mbtfs_open(void *st, char *path, mbtfs_handle_t *ret_handle) {
    
    errval_t err = SYS_ERR_OK;
    
    assert(ret_handle != NULL);
    
    // skip leading /
    size_t pos = 0;
    if (path[0] == FS_PATH_SEP) {
        pos++;
    }
    
    struct capref frame;
    size_t ret_bytes = 0;
    
    //err = aos_rpc_get_module_frame(path[pos], &frame, &ret_bytes);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Buffer for file data
    void *buffer;
    
    // Calculate frame size
    size_t frame_size = (ret_bytes + (BASE_PAGE_SIZE - 1)) / BASE_PAGE_SIZE;
    
    // Map frame to virtual address space
    err = paging_map_frame_attr(get_current_paging_state(), &buffer, frame_size, frame, VREGION_FLAGS_READ, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Allocate return header
    struct mbtfs_handle *handle = calloc(1, sizeof(struct mbtfs_handle));
    
    // Set values of handle
    handle->path = strdup(&path[pos]);
    handle->isdir = false;
    handle->pos = 0;
    handle->frame = frame;
    handle->buffer = buffer;
    handle->size = ret_bytes;
    
    // Set return handle
    *ret_handle = handle;
    
    return err;
    
}

errval_t mbtfs_create(void *st, char *path, mbtfs_handle_t *rethandle) {
    
    debug_printf("Multiboot Filesystem is read only!\n");
    
    return SYS_ERR_OK;
    
}

errval_t mbtfs_remove(void *st, char *path) {
    
    debug_printf("Multiboot Filesystem is read only!\n");
    
    return SYS_ERR_OK;

}

errval_t mbtfs_read(void *st, mbtfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read) {
    
    struct mbtfs_handle *h = handle;
    
    // File size
    size_t file_size = h->size;
    
    // Start index of requested range
    size_t start = h->pos;
    
    // Return if requested file range is not overlapping
    if (start >= file_size) {
        
        debug_printf("handler position is out of bounds\n");
        return SYS_ERR_OK;
        
    }
    
    // Shorten bytes if requested file range is bigger than file
    if (start + bytes > file_size) {
        
        bytes -= start + bytes - file_size;
        
    }
    
    // Copy requested amount of bytes from handle buffer to buffers
    memcpy(buffer, h->buffer, start + bytes);
    
    // Set return bytes read
    *bytes_read = bytes;
    
    return SYS_ERR_OK;
    
}

errval_t mbtfs_write(void *st, mbtfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_written) {
    
    debug_printf("Multiboot Filesystem is read only!\n");
    
    return SYS_ERR_OK;

}

errval_t mbtfs_truncate(void *st, mbtfs_handle_t handle, size_t bytes) {
    
    debug_printf("Multiboot Filesystem is read only!\n");
    
    return SYS_ERR_OK;
    
}

errval_t mbtfs_tell(void *st, mbtfs_handle_t handle, size_t *pos) {
    
    struct mbtfs_handle *h = handle;
    
    // Position of handle
    *pos = h->pos;
    
    return SYS_ERR_OK;
    
}

errval_t mbtfs_stat(void *st, mbtfs_handle_t handle, struct fs_fileinfo *info) {
    return SYS_ERR_OK;
}

errval_t mbtfs_seek(void *st, mbtfs_handle_t handle, enum fs_seekpos whence, off_t offset) {
    return SYS_ERR_OK;
}

errval_t mbtfs_close(void *st, mbtfs_handle_t handle) {
    
    errval_t err;
    
    assert(handle != NULL);
    
    struct mbtfs_handle *h = handle;
    
    // Free path of handle
    free(h->path);
    
    // Unmap handle buffer
    err = paging_unmap(get_current_paging_state(), h->buffer);
    
    // Delete handle frame
    cap_delete(h->frame);
    
    // Free slot of handle frame
    slot_free(h->frame);
    
    // Free handle
    free(h);
    
    return err;
    
}

errval_t mbtfs_opendir(void *st, char *path, mbtfs_handle_t *ret_handle) {
    
    assert(ret_handle != NULL);
    
    struct mbtfs_mount *mt = st;
    
    // Allocate directory handle
    struct mbtfs_handle *dh = calloc(1, sizeof(struct mbtfs_handle));
    
    // Set values of directory handle
    dh->path = strdup(mt->name);
    dh->isdir = true;
    dh->pos = 0;
    dh->frame = NULL_CAP;
    dh->buffer = NULL;
    dh->size = mt->module_count;
    
    // Set return directory handle
    *ret_handle = dh;

    return SYS_ERR_OK;

}

//vfs_readdir(mount, h, name, NULL);
errval_t mbtfs_dir_read_next(void *st, mbtfs_handle_t handle, char **retname, struct fs_fileinfo *info) {
    
    assert(retname != NULL);
    
    // Multiboot Filesystem mount to get module names
    struct mbtfs_mount *mt = st;
    
    // Directory handle
    struct mbtfs_handle *dh = handle;

    if (dh->pos < mt->module_count) {
        
        // Allocate and set return name
        *retname = strdup(mt->modules[dh->pos]);
        
        // Update directory handle position
        dh->pos += 1;
        
    } else {
        
        // Set return name to NULL if position out of bounds
        *retname = NULL;
        
        debug_printf("%s\n", err_getstring(FS_ERR_INDEX_BOUNDS));
        
        return FS_ERR_INDEX_BOUNDS;
        
    }
    
    if (info != NULL) {
        
        // TODO: Set filesystem info
        
    }

    return SYS_ERR_OK;
    
}

errval_t mbtfs_closedir(void *st, mbtfs_handle_t dirhandle) {
    
    assert(dirhandle != NULL);
    
    struct mbtfs_handle *dh = dirhandle;
    
    // Free path of directory handle
    free(dh->path);
    
    // Free directory handle
    free(dh);
    
    return SYS_ERR_OK;
    
}

errval_t mbtfs_mkdir(void *st, char *path) {
    
    debug_printf("Multiboot Filesystem is read only!\n");

    return SYS_ERR_OK;
    
}

errval_t mbtfs_rmdir(void *st, char *path) {
    
    debug_printf("Multiboot Filesystem is read only!\n");

    return SYS_ERR_OK;

}

errval_t mbtfs_mount(const char *uri, mbtfs_mount_t *retst) {
    
    errval_t err;
    
    struct capref frame;
    size_t frame_size = 0;
    size_t module_count = 0;
    
    // Module name array
    char **modules = NULL;
    
    // Get RPC multiboot module list
    err = aos_rpc_get_multiboot_module_list(aos_rpc_get_init_channel(), &modules, &module_count);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
   
    
    // Map frame to virtual address space
    err = paging_map_frame_attr(get_current_paging_state(), module, frame_size, frame, VREGION_FLAGS_READ, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    
    
    // create mbtfs_mount root
    
    return SYS_ERR_OK;
    
}
