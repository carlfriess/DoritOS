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

errval_t run_rpc_serv(void) {
    
    errval_t err;
    
    struct fatfs_serv_mount mount;
    
    // Initialize root directory for fatfs_serv
    err = init_root_dir((void *) &mount);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    domainid_t pid = 0;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "filereader", 0, &pid);
    
    // Accept a binding request from a client
    struct urpc_chan chan;
    err = urpc_accept_blocking(&chan);
    assert(err_is_ok(err));
    
    // Receive request message from client
    uint8_t *recv_buffer;
    size_t recv_size;
    urpc_msg_type_t recv_msg_type;
    
    // Send request message to client
    size_t send_size;
    uint8_t *send_buffer;
    
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    while (true) {
        
        // Wait for fs_message from client
        urpc_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
        
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
                
                debug_printf("URPC Message Open Request!\n");
                
                // Copy path string from recieve buffer
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                //int flags = recv_msg->arg1;
                
                // Open existing file and return dirent
                err = fatfs_serv_open((void *) &mount, path, &dirent);
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
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Open);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_Create:
                
                debug_printf("URPC Message Creates Request!\n");
                
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                //int flags = recv_msg->arg1;
                
                // Create existing file and return dirent
                err = fatfs_serv_create((void *) &mount, path, &dirent);
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
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Create);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_Close:
                
                debug_printf("URPC Message Close Request!\n");
                
                debug_printf("NOT IMPLEMENTED YET\n");
                
                break;
                
            case URPC_MessageType_Read:

                debug_printf("URPC Message Read Request!\n");
                
                // Set start and bytes
                start = recv_msg->arg1;
                bytes = recv_msg->arg2;

                memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
                
                size_t bytes_read = 0;

                // Size of send buffer
                send_size = sizeof(struct fs_message) + bytes;
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Read dirent data into buffer part of send buffer
                err = read_dirent(dirent, send_buffer + sizeof(struct fs_message), start, bytes, &bytes_read);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Set bytes read
                ((struct fs_message *) send_buffer)->arg2 = bytes_read;
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Read);

                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_Write:
                
                debug_printf("URPC Message Write Request!\n");
                
                start = recv_msg->arg1;
                bytes = recv_msg->arg2;
                
                memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
                
                size_t bytes_written = 0;
                
                // Write buffer part of receive buffer to dirent
                err = write_dirent(dirent, recv_buffer + sizeof(struct fs_message) + sizeof(struct fat_dirent), start, bytes, &bytes_written);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
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
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Write);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_Truncate:
                
                debug_printf("URPC Message Truncate Request!\n");
                
                bytes = recv_msg->arg1;

                memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));

                // Truncate dirent to size bytes
                err = truncate_dirent(dirent, bytes);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                
                // Size of send buffer
                send_size = sizeof(struct fs_message);
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Truncate);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
                
                break;
                
            case URPC_MessageType_Remove:
                
                debug_printf("URPC Message Remove Request!\n");
                
                memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
                
                // Remove dirent from directory and set FAT entries to zero
                err = remove_dirent(dirent);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                
                // Size of send buffer
                send_size = sizeof(struct fs_message);
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_Remove);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_OpenDir:
                
                debug_printf("URPC Message Open Directory Request!\n");
                
                // Copy path string from recieve buffer
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                
                // Open directory
                err = fatfs_serv_opendir((void *) &mount, path, &dirent);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                    return err;
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
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_OpenDir);
                
                // Free send buffer
                free(send_buffer);

                break;
                
            case URPC_MessageType_ReadDir:
                
                debug_printf("URPC Message Read Directory Request!\n");

                // Set directory index
                dir_index = recv_msg->arg1;
                
                memcpy(dirent, recv_buffer + sizeof(struct fs_message), sizeof(struct fat_dirent));
                
                // Size of send buffer
                send_size = sizeof(struct fs_message) + sizeof(struct fat_dirent);
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Allocate return dirent
                struct fat_dirent *ret_dirent = calloc(1, sizeof(struct fat_dirent));
                
                // Find dirent data
                err = fatfs_serv_readdir(dirent->first_cluster_nr, dir_index, &ret_dirent);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
            
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Copy
                if (err_is_ok(err)) {
                    memcpy(send_buffer + sizeof(struct fs_message), ret_dirent, sizeof(struct fat_dirent));
                }
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_ReadDir);
                
                // Free return dirent
                free(ret_dirent);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
        case URPC_MessageType_MakeDir:

                debug_printf("URPC Message Make Directory Request!\n");
                
                // Copy path string from recieve buffer
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));

                // Make directory
                err = fatfs_serv_mkdir((void *) &mount, path);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }

                // Size of send buffer
                send_size = sizeof(struct fs_message);
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_MakeDir);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case URPC_MessageType_RemoveDir:
                
                debug_printf("URPC Message Remove Directory Request!\n");

                // Copy path string from recieve buffer
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                
                // Make directory
                err = fatfs_serv_rmdir((void *) &mount, path);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                
                // Size of send buffer
                send_size = sizeof(struct fs_message);
                
                // Allocate send buffer
                send_buffer = calloc(1, send_size);
                
                // Set error
                ((struct fs_message *) send_buffer)->arg1 = err;
                
                // Send response message to client
                urpc_send(&chan, send_buffer, send_size, URPC_MessageType_RemoveDir);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            default:
                
                debug_printf("URPC Message Unknown Request!\n");

                debug_printf("Received Message Type: %d", recv_msg_type);
                
                continue;
                
                break;
        }
        
        memset(dirent, 0, sizeof(struct fat_dirent));
        
        
        // Free receive buffer
        free(recv_buffer);

        
    }
    
    return err;
    
}

