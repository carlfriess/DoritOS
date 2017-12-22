//
//  fs_rpc.c
//  DoritOS
//

#include <stdio.h>
#include <string.h>

#include <aos/aos.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>


#include <fs/fs.h>
#include "fs_internal.h"
//#include <fs/fopen.c>

#include <fs/fs_rpc.h>
#include <fs_serv/fat_helper.h>

//typedef struct fat32fs_handle *fat32fs_handle_t;


static struct fat32fs_handle *handle_open(struct fat_dirent *dirent);
static void handle_close(struct fat32fs_handle *handle);


static struct urpc_chan chan;

errval_t fs_rpc_init(void *state) {

    errval_t err;
    
    // Find mmchs's pid
    //  FIXME: Pid discovery gets weirdly stuck
    domainid_t pid;
    err = aos_rpc_process_get_pid_by_name("mmchs", &pid);
    if (err_is_fail(err)) {
        return err;
    }

    // Try to bind to mmchs
    //  Use LMP when on core 0!
    err = urpc_bind(pid, &chan, !disp_get_core_id());
    if (err_is_fail(err)) {
        return err;
    }
    
    return SYS_ERR_OK;

}


/// -> [fs_message] | [path]
/// <- [fs_message] | [dirent]
errval_t fs_rpc_open(void *st, char *path, fat32fs_handle_t *ret_handle) {
    
    errval_t err;
    
    assert(ret_handle != NULL);
    
    //struct ramfs_mount *mount = st;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Open);

    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_Open);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Allocate and copy in dirent from receive buffer
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));

    // Not necessary since constant size
    //dirent->name = strdup(dirent->name);
    
    // Construct/open handle
    struct fat32fs_handle *handle = handle_open(dirent);
    
    // Copy in path string
    handle->path = strdup(path);
    
    // Set return handle
    *ret_handle = handle;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}

/// -> [fs_message] | [path]
/// <- [fs_message] | [dirent]
errval_t fs_rpc_create(void *st, char *path, fat32fs_handle_t *ret_handle) {
    
    errval_t err;
    
    assert(ret_handle != NULL);
    
    //struct ramfs_mount *mount = st;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Create);
    
    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_Create);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Allocate and copy in dirent from receive buffer
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
    
    // Not necessary since constant size
    //dirent->name = strdup(dirent->name);
    
    // Construct/open handle
    struct fat32fs_handle *handle = handle_open(dirent);
    
    // Copy in path string
    handle->path = strdup(path);

    
    // Construct handle LEGACY
    //struct fat32fs_handle *handle = calloc(1, sizeof(struct fat32fs_handle));
    //handle->common = NULL;
    
    // Set return handle
    *ret_handle = handle;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;

}


// -> [fs_message] | [path]
// <- [fs_message]
errval_t fs_rpc_remove(void *st, char *path)
{
    errval_t err;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Remove);
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);

    assert(recv_msg_type == URPC_MessageType_Remove);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Set error
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}

/// -> [fs_message] | [dirent]
/// <- [fs_message] | [buffer]
errval_t fs_rpc_read(void *st, fat32fs_handle_t handle, void *buffer, size_t bytes,
                      size_t *bytes_read) {
    
    errval_t err;
    
    struct fat32fs_handle *h = handle;
    
    assert(bytes_read != NULL);

    assert(handle != NULL);
    
    //struct fat32_handle *h = handle;
    
    if (h->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    assert(h->pos >= 0);
    
    struct fs_message send_msg = {
        .arg1 = h->pos,
        .arg2 = bytes,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy dirent into send buffer
    memcpy(send_buffer + sizeof(struct fs_message), h->dirent, sizeof(struct fat_dirent));
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Read);
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_Read);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Set error
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Read bytes by server
    size_t ret_bytes = recv_msg->arg2;
    
    assert(ret_bytes + sizeof(struct fs_message) <= recv_size);
    
    // Update file position
    h->pos += ret_bytes;
    
    // Copy read data from receive buffer into buffer
    memcpy(buffer, recv_buffer + sizeof(struct fs_message), ret_bytes);
    
    // Set return argument bytes read by server
    *bytes_read = ret_bytes;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}

/// -> [fs_message] | [dirent] | [buffer]
/// <- [fs_message] | [buffer]
errval_t fs_rpc_write(void *st, fat32fs_handle_t handle, const void *buffer,
                       size_t bytes, size_t *bytes_written) {
    
    errval_t err;
    
    struct fat32fs_handle *h = handle;
    
    assert(bytes_written != NULL);
    
    assert(handle != NULL);
    
    //struct fat32_handle *h = handle;
    
    if (h->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    assert(h->pos >= 0);
    
    struct fs_message send_msg = {
        .arg1 = h->pos,
        .arg2 = bytes,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent) + bytes;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy dirent into send buffer
    memcpy(send_buffer + sizeof(struct fs_message), h->dirent, sizeof(struct fat_dirent));
    
    // Copy buffer into send buffer
    memcpy(send_buffer + sizeof(struct fs_message) + sizeof(struct fat_dirent), buffer, bytes);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Write);
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_Write);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Set error
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Read bytes by server
    size_t ret_bytes = recv_msg->arg2;
    
    // Update file position
    h->pos += ret_bytes;
    
    // Set return argument bytes written by server
    *bytes_written = ret_bytes;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}

/// -> [fs_message] | [dirent]
/// <- [fs_message]
errval_t fs_rpc_truncate(void *st, fat32fs_handle_t handle, size_t bytes) {
    
    errval_t err;
    
    struct fat32fs_handle *h = handle;
    
    struct fs_message send_msg = {
        .arg1 = bytes,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);

    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), h->dirent, sizeof(struct fat32fs_handle));
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Truncate);
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_Truncate);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Set error
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}



// TODO: handle void pointer?
errval_t fs_rpc_tell(void *st, fat32fs_handle_t handle, size_t *ret_pos) {
    
    assert(handle != NULL);
    assert(ret_pos != NULL);
    
    struct fat32fs_handle *h = handle;
    
    //struct fat32_handle *h = handle;
    
    // Check if handle is directory or file
    if (h->isdir) {
        
        // Set pos to 0
        *ret_pos = 0;
        //*pos = handle->pos;       // TODO: Why not?

    
    } else {
        
        // Set return position
        *ret_pos = h->pos;
    
    }
    
    return SYS_ERR_OK;

}

// TODO: inhandle void pointer?
errval_t fs_rpc_stat(void *st, fat32fs_handle_t inhandle, struct fs_fileinfo *info) {
    
    struct fat32fs_handle *h = inhandle;
    
    // FIXME: MAKE RPC CALL!!!
    
    assert(h != NULL);
    
    assert(info != NULL);
    info->type = h->isdir ? FS_DIRECTORY : FS_FILE;
    info->size = h->dirent->size;
    
    return SYS_ERR_OK;
    
}

// TODO: handle void pointer?
errval_t fs_rpc_seek(void *st, fat32fs_handle_t handle, enum fs_seekpos whence, off_t offset) {

    errval_t err = SYS_ERR_OK;
    
    struct fat32fs_handle *h = handle;
    
    // File information
    struct fs_fileinfo info;
    
    switch (whence) {
        case FS_SEEK_SET:
            
            assert(offset >= 0);
            h->pos = offset;
            
            break;
            
        case FS_SEEK_CUR:
            
            assert(offset >= 0 || -offset <= h->pos);
            h->pos += offset;
            
            break;
            
        case FS_SEEK_END:
            
            // TODO: Implement directory version
            
            err = fs_rpc_stat(st, handle, &info);
            if (err_is_fail(err)) {
                return err;
            }
            assert(offset >= 0 || -offset <= info.size);
            h->pos = info.size + offset;
            
            break;
            
        default:
            USER_PANIC("invalid whence argument to ramfs seek");
    }
    
    return err;
    
}

// TODO: inhandle void pointer?
errval_t fs_rpc_close(void *st, fat32fs_handle_t inhandle)
{
    
    struct fat32fs_handle *h = inhandle;
    
    //struct fat32fs_handle *handle = inhandle;
    assert(h != NULL);
    
    if (h->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    handle_close(h);
    
    return SYS_ERR_OK;

}

errval_t fs_rpc_opendir(void *st, char *path, fs_dirhandle_t *ret_dirhandle) {
    
    errval_t err;
    
    //struct ramfs_mount *mount = st;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_OpenDir);
    
    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_OpenDir);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Allocate and copy in dirent from receive buffer
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
    
    // Construct/open handle
    struct fat32fs_handle *handle = handle_open(dirent);
    
    // Copy in path string
    handle->path = strdup(path);
    
    // Set return handle
    *ret_dirhandle = handle;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}



errval_t fs_rpc_readdir(void *st, fs_dirhandle_t dirhandle, char **ret_name,
                        struct fs_fileinfo *info) {
    
    errval_t err;
    
    struct fat32fs_handle *handle = dirhandle;
    
    assert(ret_name != NULL);
    
    //assert(info != NULL);
    
    assert(handle != NULL);
    
    if (!handle->isdir) {
        return FS_ERR_NOTDIR;
    }
    
    assert(handle->pos >= 0);
    
    struct fs_message send_msg = {
        .arg1 = handle->pos,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy dirent into send buffer
    memcpy(send_buffer + sizeof(struct fs_message), handle->dirent, sizeof(struct fat_dirent));
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_ReadDir);
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_ReadDir);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Set error
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Check if directory position pointer reached the end
    if (err == FS_ERR_INDEX_BOUNDS) {
#if PRINT_DEBUG
        debug_printf("Reached end of directory\n");
#endif
        // Set return name to NULL
        *ret_name = NULL;
        
        // Set return file system info to NULL
        //info->type = ???;
        //info->size = ???;
                
        // Free receive buffer
        free(recv_buffer);
        
        return err;
        
    }
    
    // Update directory index
    handle->pos += 1;
    
    // FIXME: Maybe make dirent not calloc
    
    // Allocate and copy in dirent from receive buffer
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Copy read data from receive buffer into buffer
    memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));

    // Set return file system info accordingly
    if (info != NULL) {
        info->type = dirent->is_dir ? FS_DIRECTORY : FS_FILE;
        info->size = dirent->size;
    }
    
    // Set return argument bytes read by server
    *ret_name = convert_to_normal_name(dirent->name);
    
    // Free dirent
    free(dirent);
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}


errval_t fs_rpc_closedir(void *st, fs_dirhandle_t dirhandle) {
    
    struct fat32fs_handle *handle = dirhandle;
    
    assert(handle != NULL);
    
    if (!handle->isdir) {
        return FS_ERR_NOTDIR;
    }
    
    handle_close(dirhandle);
    
    return SYS_ERR_OK;

}


/// -> [fs_message] | [path]
/// <- [fs_message]
errval_t fs_rpc_mkdir(void *st, char *path) {
    
    errval_t err;
    
    //struct ramfs_mount *mount = st;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;

    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);

    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));

    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);

    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_MakeDir);
    
    // TODO: Free send buffer?

    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_MakeDir);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}


/// -> [fs_message] | [path]
/// <- [fs_message]
errval_t fs_rpc_rmdir(void *st, char *path) {
    
    debug_printf("path to be removed: %s\n", path);
    
    errval_t err;
    
    //struct ramfs_mount *mount = st;
    
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    // Length of string path (including '\0')
    size_t path_size = strlen(path) + 1;
    
    // Size of send buffer
    size_t send_size = sizeof(struct fs_message) + path_size;
    
    // Allocate send buffer
    uint8_t *send_buffer = calloc(1, send_size);
    
    // Copy fs_message into send buffer
    memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
    
    // Copy path (including '\0' into send buffers
    memcpy(send_buffer + sizeof(struct fs_message), path, path_size);
    
    // Send request message to server
    urpc_send(&chan, send_buffer, send_size, URPC_MessageType_RemoveDir);
    
    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == URPC_MessageType_RemoveDir);
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    err = recv_msg->arg1;
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}





static struct fat32fs_handle *handle_open(struct fat_dirent *dirent)
{
    struct fat32fs_handle *handle = calloc(1, sizeof(struct fat32fs_handle));
    if (handle == NULL) {
        return NULL;
    }
    
    //handle->common = NULL;
    handle->isdir = dirent->is_dir;
    handle->dirent = dirent;
    handle->pos = 0;

    return handle;
}

static void handle_close(struct fat32fs_handle *handle)
{
    //assert(h->dirent->refcount > 0);
    //h->dirent->refcount--;
    //free(handle->dirent);
    free(handle->path);
    free(handle);
}






