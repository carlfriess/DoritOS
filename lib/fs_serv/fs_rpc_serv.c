//
//  fs_rpc_serv.c
//  DoritOS
//

#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/ump.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>

#include <fs_serv/fs_fat_serv.h>

#include <fs_serv/fs_rpc_serv.h>

#include <fs/fs_rpc.h>

/*
#define UMP_MessageType_Open      UMP_MessageType_User0
#define UMP_MessageType_Close     UMP_MessageType_User1
#define UMP_MessageType_Read      UMP_MessageType_User2
#define UMP_MessageType_Write     UMP_MessageType_User3
#define UMP_MessageType_Add       UMP_MessageType_User4
#define UMP_MessageType_Remove    UMP_MessageType_User5
#define UMP_MessageType_Create    UMP_MessageType_User6
*/

errval_t run_rpc_serv(void) {
    
    errval_t err;
    
    domainid_t pid = 0;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "hello", 0, &pid);
    
    struct fat32_mount mount;
    
    err = init_root_dir((void *) &mount);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Accept a binding request from a client
    struct ump_chan chan;
    err = urpc_accept(&chan);
    assert(err_is_ok(err));
    
    // Variable declarations
    //uint32_t counter;
    //size_t size;
    //void *ptr;
    
    // Receive request message from client
    uint8_t *recv_buffer;
    size_t recv_size;
    ump_msg_type_t recv_msg_type;
    
    // Send request message to client
    size_t send_size;
    uint8_t *send_buffer;
    
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    while (true) {
        
        // Wait for fs_message from client
        ump_recv_blocking(&chan, (void **) &recv_buffer, &recv_size, &recv_msg_type);
        
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
            
            case UMP_MessageType_Open:
                
                debug_printf("UMP Message Open Request!\n");
                
                // Copy path string from recieve buffer
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                //int flags = recv_msg->arg1;
                
                // Open existing file and return dirent
                err = fat_open((void *) &mount, path, &dirent);
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
                ump_send(&chan, send_buffer, send_size, UMP_MessageType_Open);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case UMP_MessageType_Create:
                
                debug_printf("UMP Message Creates Request!\n");
                
                path = strdup((char *) (recv_buffer + sizeof(struct fs_message)));
                //int flags = recv_msg->arg1;
                
                // Create existing file and return dirent
                err = fat_create((void *) &mount, path, &dirent);
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
                ump_send(&chan, send_buffer, send_size, UMP_MessageType_Create);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case UMP_MessageType_Close:
                
                debug_printf("UMP Message Close Request!\n");
                
                debug_printf("NOT IMPLEMENTED YET\n");
                
                break;
                
            case UMP_MessageType_Read:

                debug_printf("UMP Message Read Request!\n");
                
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
                ump_send(&chan, send_buffer, send_size, UMP_MessageType_Read);

                // Free send buffer
                free(send_buffer);
                
                break;
                
            case UMP_MessageType_Write:
                
                debug_printf("UMP Message Write Request!\n");
                
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
                ump_send(&chan, send_buffer, send_size, UMP_MessageType_Write);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            case UMP_MessageType_Add:
                
                debug_printf("UMP Message Add Request!\n");
                
                debug_printf("NOT IMPLEMENTED YET\n");
                
                break;
                
            case UMP_MessageType_Remove:
                
                debug_printf("UMP Message Remove Request!\n");
                
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
                ump_send(&chan, send_buffer, send_size, UMP_MessageType_Remove);
                
                // Free send buffer
                free(send_buffer);
                
                break;
                
            default:
                
                debug_printf("UMP Message Unknown Request!\n");

                debug_printf("Received Message Type: %d", recv_msg_type);
                
                break;
        }
        
        
        // TODO: Free receive buffer?
        
    }
    
    return err;
    
}

