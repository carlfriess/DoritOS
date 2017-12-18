//
//  fs_rpc.c
//  DoritOS
//

#include <stdio.h>
#include <string.h>

#include <aos/aos.h>
#include <aos/ump.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>


#include <fs/fs.h>
#include "fs_internal.h"
//#include <fs/fopen.c>


#include <fs/fs_rpc.h>

//typedef struct fat32fs_handle *fat32fs_handle_t;


static struct fat32fs_handle *handle_open(struct fat_dirent *dirent);
static void handle_close(struct fat32fs_handle *handle);


static struct ump_chan chan;

errval_t fs_rpc_init(void *state) {
    
    errval_t err;

    struct aos_rpc *rpc_chan = aos_rpc_get_init_channel();

    domainid_t pid = 0;
    
    // Find fs_service (mmchs) pid
    {
        
        // Request the pids of all running processes
        domainid_t *pids;
        size_t num_pids;
        err = aos_rpc_process_get_all_pids(rpc_chan, &pids, &num_pids);
        assert(err_is_ok(err));
        
        
        // Iterate pids and compare names
        for (int i = 0; i < num_pids; i++) {
            
            // Get the process name
            char *name;
            err = aos_rpc_process_get_name(rpc_chan, pids[i], &name);
            assert(err_is_ok(err));
            
            // Compare the name
            if (!strcmp("mmchs", name)) {
                pid = pids[i];
                printf("Found mmchs, PID: %d\n", pid);
                free(name);
                break;
            }
            free(name);
        }
        
        // Make sure bind_server was found
        assert(pid != 0 && "mmchs NOT FOUND");
    }
    
    // Bind to the server
    err = urpc_bind(pid, &chan);
    assert(err_is_ok(err));
    
    return err;

}

/*
/// -> [fs_message] | [path]
/// <- [fs_message] | [dirent]
errval_t fs_rpc_mount(const char *path, const char *uri) {
    
    
    
    return LIB_ERR_NOT_IMPLEMENTED;
    
}
*/

/// -> [fs_message] | [path]
/// <- [fs_message] | [dirent]
errval_t fs_rpc_open(void *st, const char *path, struct fat32fs_handle **ret_handle) {
    
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
    ump_send(&chan, send_buffer, send_size, UMP_MessageType_Open);

    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == UMP_MessageType_Open);
    
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
errval_t fs_rpc_create(void *st, const char *path, struct fat32fs_handle **ret_handle) {
    
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
    ump_send(&chan, send_buffer, send_size, UMP_MessageType_Create);
    
    // TODO: Free send buffer?
    
    // Receive response message from server
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == UMP_MessageType_Create);
    
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
errval_t fs_rpc_remove(void *st, const char *path)
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
    ump_send(&chan, send_buffer, send_size, UMP_MessageType_Remove);
    
    // Receive response message from server
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);

    assert(recv_msg_type == UMP_MessageType_Remove);
    
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
errval_t fs_rpc_read(void *st, struct fat32fs_handle *handle, void *buffer, size_t bytes,
                      size_t *bytes_read) {
    
    errval_t err;
    
    assert(bytes_read != NULL);

    assert(handle != NULL);
    
    //struct fat32_handle *h = handle;
    
    if (handle->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    assert(handle->file_pos >= 0);
    
    struct fs_message send_msg = {
        .arg1 = handle->file_pos,
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
    memcpy(send_buffer + sizeof(struct fs_message), handle->dirent, sizeof(struct fat_dirent));
    
    // Send request message to server
    ump_send(&chan, send_buffer, send_size, UMP_MessageType_Read);
    
    // Receive response message from server
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == UMP_MessageType_Read);
    
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
    handle->file_pos += ret_bytes;
    
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
errval_t fs_rpc_write(void *st, struct fat32fs_handle *handle, const void *buffer,
                       size_t bytes, size_t *bytes_written) {
    
    errval_t err;
    
    assert(bytes_written != NULL);
    
    assert(handle != NULL);
    
    //struct fat32_handle *h = handle;
    
    if (handle->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    assert(handle->file_pos >= 0);
    
    struct fs_message send_msg = {
        .arg1 = handle->file_pos,
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
    memcpy(send_buffer + sizeof(struct fs_message), handle->dirent, sizeof(struct fat_dirent));
    
    // Send request message to server
    ump_send(&chan, send_buffer, send_size, UMP_MessageType_Write);
    
    // Receive response message from server
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    uint8_t *recv_buffer;
    
    // Wait for response from server
    ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
    
    assert(recv_msg_type == UMP_MessageType_Write);
    
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
    handle->file_pos += ret_bytes;
    
    // Set return argument bytes written by server
    *bytes_written = ret_bytes;
    
    // Free receive buffer
    free(recv_buffer);
    
    return err;
    
}

// TODO:
//errval_t ramfs_truncate(void *st, ramfs_handle_t handle, size_t bytes);



// TODO: handle void pointer?
errval_t fs_rpc_tell(void *st, struct fat32fs_handle *handle, size_t *pos) {
    
    assert(handle != NULL);
    
    //struct fat32_handle *h = handle;
    
    // Check if handle is directory or file
    if (handle->isdir) {
        
        // Set pos to 0
        *pos = 0;
    
    } else {
        
        // Set pos to file_pos of handle
        *pos = handle->file_pos;
    
    }
    
    return SYS_ERR_OK;

}

// TODO: inhandle void pointer?
errval_t fs_rpc_stat(void *st, struct fat32fs_handle *inhandle, struct fs_fileinfo *info) {
    
    //struct fat32fs_handle *h = inhandle;
    
    assert(inhandle != NULL);
    
    assert(info != NULL);
    info->type = inhandle->isdir ? FS_DIRECTORY : FS_FILE;
    info->size = inhandle->dirent->size;
    
    return SYS_ERR_OK;
    
}

// TODO: handle void pointer?
errval_t fs_rpc_seek(void *st, struct fat32fs_handle *handle, enum fs_seekpos whence, off_t offset) {

    //errval_t err;
    
    //struct ramfs_handle *h = handle;
    
    // File information
    //struct fs_fileinfo info;
    /*
    switch (whence) {
        case FS_SEEK_SET:
            
            assert(offset >= 0);
            
            if (h->isdir) {
                h->dir_pos = h->dirent->parent->dir;
                for (size_t i = 0; i < offset; i++) {
                    if (h->dir_pos  == NULL) {
                        break;
                    }
                    h->dir_pos = h->dir_pos->next;
                }
            } else {
                h->file_pos = offset;
            }
            break;
            
        case FS_SEEK_CUR:
            if (h->isdir) {
                assert(!"NYI");
            } else {
                assert(offset >= 0 || -offset <= h->file_pos);
                h->file_pos += offset;
            }
            
            break;
            
        case FS_SEEK_END:
            if (h->isdir) {
                assert(!"NYI");
            } else {
                err = ramfs_stat(st, handle, &info);
                if (err_is_fail(err)) {
                    return err;
                }
                assert(offset >= 0 || -offset <= info.size);
                h->file_pos = info.size + offset;
            }
            break;
            
        default:
            USER_PANIC("invalid whence argument to ramfs seek");
    }
    */
    //return SYS_ERR_OK;
    
    return LIB_ERR_NOT_IMPLEMENTED;
    
}

// TODO: inhandle void pointer?
errval_t fs_rpc_close(void *st, struct fat32fs_handle *inhandle)
{
    //struct fat32fs_handle *handle = inhandle;
    assert(inhandle != NULL);
    
    if (inhandle->isdir) {
        return FS_ERR_NOTFILE;
    }
    
    handle_close(inhandle);
    
    return SYS_ERR_OK;

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
    handle->file_pos = 0;

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






