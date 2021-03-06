//
//  fatfs_rpc_serv.c
//  DoritOS
//

#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>

#include <fs_serv/fatfs_serv.h>
#include <fs_serv/fatfs_rpc_serv.h>

#include <fs/fs_rpc.h>

#include <collections/list.h>

#define PRINT_DEBUG 0

// List of bound channels
static collections_listnode *chan_list;


static void handle_urpc_msg(struct urpc_chan *chan,
                            uint8_t *recv_buffer,
                            size_t recv_size,
                            urpc_msg_type_t recv_msg_type,
                            struct fatfs_serv_mount *mt) {
    
    errval_t err;
    
    // Send request message to client
    size_t send_size;
    uint8_t *send_buffer;
    
    // Allocate dirent
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Receive header response message
    struct fs_message *recv_msg = (struct fs_message *) recv_buffer;
    
    // Size and bytes for read and write
    size_t start;
    size_t bytes;
    
    // Directory index for readdir
    size_t dir_index;
    
    // Path for open and create
    char *path;
    
    // File system message
    struct fs_message send_msg = {
        .arg1 = 0,
        .arg2 = 0,
        .arg3 = 0,
        .arg4 = 0
    };
    
    switch (recv_msg_type) {
            
        case URPC_MessageType_Open:
#if PRINT_DEBUG
            debug_printf("URPC Message Open Request!\n");
#endif
            // Copy path string from recieve buffer
            path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
            //int flags = recv_msg->arg1;
            
            // Open existing file and return dirent
            err = fatfs_serv_open((void *) mt, path, &dirent);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            
            // Free path string
            free(path);
            
            // Set err as argument 1 of send message
            send_msg.arg1 = err;
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Copy fs_message into send buffer
            memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
            
            // Copy dirent into send buffers
            memcpy(send_buffer + sizeof(struct fs_message), dirent, sizeof(struct fat_dirent));
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Open);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_Create:
#if PRINT_DEBUG
            debug_printf("URPC Message Creates Request!\n");
#endif
            path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
            //int flags = recv_msg->arg1;
            
            // Create existing file and return dirent
            err = fatfs_serv_create((void *) mt, path, &dirent);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            
            // Free path string
            free(path);
            
            // Set err as argument 1 of send message
            send_msg.arg1 = err;
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Copy fs_message into send buffer
            memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
            
            // Copy path (including '\0' into send buffers
            memcpy(send_buffer + sizeof(struct fs_message), dirent, sizeof(struct fat_dirent));
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Create);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_Close:
#if PRINT_DEBUG
            debug_printf("URPC Message Close Request!\n");
#endif
            break;
            
        case URPC_MessageType_Read:
#if PRINT_DEBUG
            debug_printf("URPC Message Read Request!\n");
#endif
            // Set start and bytes
            start = recv_msg->arg1;
            bytes = recv_msg->arg2;
            
            // Copy from buffer into dirents
            memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
            
            // Update dirent size
            err = update_dirent_size(dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            // Setup bytes that will be read
            size_t bytes_read = 0;
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + bytes;
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Read dirent data into buffer part of send buffer
            err = read_dirent(dirent, send_buffer + sizeof(struct fs_message), start, bytes, &bytes_read);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Set bytes read
            ((struct fs_message *) send_buffer)->arg2 = bytes_read;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Read);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_Write:
#if PRINT_DEBUG
            debug_printf("URPC Message Write Request!\n");
#endif
            // Get start and bytes from arguments
            start = recv_msg->arg1;
            bytes = recv_msg->arg2;
            
            // Copy from receive buffer into dirent
            memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
            
            // Update dirent size
            err = update_dirent_size(dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Setup bytes that will be written
            size_t bytes_written = 0;
            
            // Write buffer part of receive buffer to dirent
            err = write_dirent(dirent, recv_buffer + sizeof(struct fs_message) + sizeof(struct fat_dirent), start, bytes, &bytes_written);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + bytes;
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Set bytes written
            ((struct fs_message *) send_buffer)->arg2 = bytes_written;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Write);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_Truncate:
#if PRINT_DEBUG
            debug_printf("URPC Message Truncate Request!\n");
#endif

            // Get bytes to be truncated from arguments
            bytes = recv_msg->arg1;
            
            // Copy from receive buffer into dirents
            memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
      
            // Update dirent size
            err = update_dirent_size(dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Truncate dirent to size bytes
            err = truncate_dirent(dirent, bytes);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Size of send buffer
            send_size = sizeof(struct fs_message);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Truncate);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_Remove:
#if PRINT_DEBUG
            debug_printf("URPC Message Remove Request!\n");
#endif
        
            // Allocate and copy path (with '\0')
            path = calloc(recv_msg->arg1, sizeof(char));
            memcpy(path, recv_buffer + sizeof(struct fs_message), recv_msg->arg1);
            
            // Remove file dirent
            err = fatfs_rm_serv((void *) mt, path);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Free path
            free(path);
            
            // Size of send buffer
            send_size = sizeof(struct fs_message);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_Remove);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_OpenDir:
#if PRINT_DEBUG
            debug_printf("URPC Message Open Directory Request!\n");
#endif
            // Copy path string from recieve buffer
            path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
            
            // Open directory
            err = fatfs_serv_opendir((void *) mt, path, &dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Free path string
            free(path);
            
            // Set err as argument 1 of send message
            send_msg.arg1 = err;
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Copy fs_message into send buffer
            memcpy(send_buffer, &send_msg, sizeof(struct fs_message));
            
            // Copy dirent into send buffers
            memcpy(send_buffer + sizeof(struct fs_message), dirent, sizeof(struct fat_dirent));
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_OpenDir);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_ReadDir:
#if PRINT_DEBUG
            debug_printf("URPC Message Read Directory Request!\n");
#endif
            // Set directory index
            dir_index = recv_msg->arg1;
            
            // Copy from receive buffer into dirents
            memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
            
            // Update dirent size
            err = update_dirent_size(dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Size of send buffer
            send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Allocate return dirent
            struct fat_dirent *ret_dirent = calloc(1, sizeof(struct fat_dirent));
            
            // Find dirent data
            err = fatfs_serv_readdir(dirent->first_cluster_nr, dir_index, &ret_dirent);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Copy
            if (err_is_ok(err)) {
                memcpy(send_buffer + sizeof(struct fs_message), ret_dirent, sizeof(struct fat_dirent));
            }
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_ReadDir);
            
            // Free return dirent
            free(ret_dirent);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_MakeDir:
#if PRINT_DEBUG
            debug_printf("URPC Message Make Directory Request!\n");
#endif
            // Copy path string from recieve buffer
            path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
            
            // Make directory
            err = fatfs_serv_mkdir((void *) mt, path);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Size of send buffer
            send_size = sizeof(struct fs_message);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_MakeDir);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        case URPC_MessageType_RemoveDir:
#if PRINT_DEBUG
            debug_printf("URPC Message Remove Directory Request!\n");
#endif
            // Copy path string from recieve buffer
            path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
            
            // Make directory
            err = fatfs_serv_rmdir((void *) mt, path);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
            }
            
            // Size of send buffer
            send_size = sizeof(struct fs_message);
            
            // Allocate send buffer
            send_buffer = calloc(1, send_size);
            
            // Set error
            ((struct fs_message *) send_buffer)->arg1 = err;
            
            // Send response message to client
            urpc_send(chan, send_buffer, send_size, URPC_MessageType_RemoveDir);
            
            // Free send buffer
            free(send_buffer);
            
            break;
            
        default:
#if PRINT_DEBUG
            debug_printf("Unknown URPC Message!\n");
            
            debug_printf("Received Message Type: %d", recv_msg_type);
#endif
            break;
    }
    
    // Clean up
    free(dirent);
    
}

errval_t run_rpc_serv(void) {
    
    errval_t err;
    
    struct fatfs_serv_mount mt;
    
    // Initialize root directory for fatfs_serv
    err = init_root_dir((void *) &mt);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
    }
    
    // Initialize channel list
    collections_list_create(&chan_list, free);
    
//    domainid_t pid = 0;
//    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "filereader", 0, &pid);
    
    // Receive request message from client
    uint8_t *recv_buffer;
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    
    struct urpc_chan *new_chan = malloc(sizeof(struct urpc_chan));
    assert(new_chan);

//    domainid_t pid;
//    struct aos_rpc *init_chan = aos_rpc_get_init_channel();
//    err = aos_rpc_process_spawn(init_chan, "shell", 0, &pid);
//    if (err_is_fail(err)) {
//#if PRINT_DEBUG
//        debug_printf("%s\n", err_getstring(err));
//#endif
//    }

    while (true) {
        
        // Accept a binding request from a client
        err = urpc_accept(new_chan);
        if (err_is_ok(err)) {
            collections_list_insert(chan_list, new_chan);
            new_chan = malloc(sizeof(struct urpc_chan));
            assert(new_chan);
        }
        else if (err != LIB_ERR_NO_URPC_BIND_REQ) {
#if PRINT_DEBUG
            debug_printf("Error in urpc_accept(): %s\n", err_getstring(err));
#endif
        }
        
        // Iterate all bound URPC channels
        struct urpc_chan *chan;
        collections_list_traverse_start(chan_list);
        while ((chan = (struct urpc_chan *) collections_list_traverse_next(chan_list)) != NULL) {
            
            // Wait for fs_message from client
            err = urpc_recv(chan,
                            (void **) &recv_buffer,
                            &recv_size,
                            &recv_msg_type);
            if (err_is_ok(err)) {
                
                // Handle received message
                handle_urpc_msg(chan, recv_buffer, recv_size, recv_msg_type, &mt);
                
                // Free receive buffer
                free(recv_buffer);
                
            }
            else if (err != LIB_ERR_NO_URPC_MSG) {
#if PRINT_DEBUG
                debug_printf("Error in urpc_recv(): %s\n", err_getstring(err));
#endif
            }
            
        }
        collections_list_traverse_end(chan_list);
        
    }
    
    return err;
    
}

