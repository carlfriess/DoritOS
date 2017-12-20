#include <stdio.h>

#include <collections/list.h>

#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/lmp.h>
#include <aos/ump.h>
#include <aos/urpc.h>
#include <aos/process.h>
#include <aos/domain.h>
#include <aos/aos_rpc.h>

#define MAX_ALLOCATION 100000000

#define SHORT_BUF_SIZE 7

#define PRINT_DEBUG 0


/* MARK: - ========== Server ========== */

extern struct ump_chan init_uc;

void lmp_server_dispatcher(void *arg) {

#if PRINT_DEBUG
    debug_printf("LMP Message Received from PID %d!\n", process_pid_for_lmp_chan((struct lmp_chan *) arg));
#endif

    errval_t err;

    struct lmp_chan *lc = (struct lmp_chan *) arg;
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    err = lmp_chan_recv(lc, &msg, &cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));

        err = lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_server_dispatcher, (void *) lc));
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }

        return;
    }

    char *string;

    // Check message type and handle
    switch (msg.words[0]) {
            
            
        case LMP_RequestType_Number:
#if PRINT_DEBUG
            debug_printf("Number Message!\n");
#endif
            printf("Received number: %d\n", msg.words[1]);
            lmp_chan_send2(lc,
                           LMP_SEND_FLAGS_DEFAULT,
                           NULL_CAP,
                           LMP_RequestType_Number,
                           msg.words[1]);
            break;
            
        case LMP_RequestType_StringShort:
        case LMP_RequestType_StringLong:
       
#if PRINT_DEBUG
            debug_printf("Short or Long String Message!\n");
#endif
            lmp_recv_string_from_msg(lc, cap, msg.words, &string);
            printf("Received string: %s\n", string);
            free(string);
            break;

            
        case LMP_RequestType_SpawnShort:
        case LMP_RequestType_SpawnLong:
            
#if PRINT_DEBUG
            debug_printf("Short or Long Spawn Message!\n");
#endif
            lmp_recv_spawn_from_msg(lc, cap, msg.words, &string);
            free(string);
            break;

            
        case LMP_RequestType_Register:
#if PRINT_DEBUG
            debug_printf("Registration Message!\n");
#endif
            lmp_server_register(lc, cap);
            break;
            
            
        case LMP_RequestType_MemoryAlloc:
#if PRINT_DEBUG
            debug_printf("Memory Alloc Message!\n");
#endif
            lmp_server_memory_alloc(lc, msg.words[1], msg.words[2]);
            break;
            
            
        case LMP_RequestType_MemoryFree:
#if PRINT_DEBUG
            debug_printf("Memory Free Message!\n");
#endif
            break;
            
            
        case LMP_RequestType_NameLookup:
#if PRINT_DEBUG
            debug_printf("Name Lookup Message!\n");
#endif
            // Send name of process
            lmp_send_string(lc, process_name_for_pid(msg.words[1]));
            break;
            
            
        case LMP_RequestType_PidDiscover:
#if PRINT_DEBUG
            debug_printf("PID Discovery Message!\n");
#endif
            lmp_server_pid_discovery(lc);
            break;
            

        case LMP_RequestType_Echo:
#if PRINT_DEBUG
            debug_printf("Echo Message!\n");
#endif
            do {
                err = lmp_chan_send8(lc,
                               LMP_SEND_FLAGS_DEFAULT,
                               cap,
                               msg.words[0],
                               msg.words[1],
                               msg.words[2],
                               msg.words[3],
                               msg.words[4],
                               msg.words[5],
                               msg.words[6],
                               msg.words[7]);
            } while (err_is_fail(err));
            break;
            
            
        case LMP_RequestType_UmpBind:
        case LMP_RequestType_LmpBind:
#if PRINT_DEBUG
            debug_printf("URPC Bind Message!\n");
#endif
            // Make a new slot available for the next incoming capability
            err = lmp_chan_alloc_recv_slot(lc);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            // Handle the request
            urpc_handle_lmp_bind_request(lc, cap, msg);
            break;
        
        case LMP_RequestType_DeviceCap:

#if PRINT_DEBUG
            debug_printf("Device Capability Message!\n");
#endif

            // Retype device capability and send that back to client
            lmp_server_device_cap(lc, msg.words[1], msg.words[2]);
            
            break;

        case LMP_RequestType_ProcessDeregister:
#if PRINT_DEBUG
            debug_printf("Process Deregister Message!\n");
#endif
            lmp_server_process_deregister(lc);
            break;

        case LMP_RequestType_ProcessDeregisterNotify:
#if PRINT_DEBUG
            debug_printf("Process Deregister Notify Message!\n");
#endif
            lmp_server_process_deregister_notify(lc, msg.words[1]);
            break;
            
        default:
#if PRINT_DEBUG
            debug_printf("Invalid Message!\n");
#endif
            break;
            
            
    }

    // Register again
    err = lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_server_dispatcher, (void *) lc));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

// SPAWN: Handle registration requests from clients
void lmp_server_register(struct lmp_chan *lc, struct capref cap) {
    errval_t err;

    lc->remote_cap = cap;

    err = lmp_chan_alloc_recv_slot(lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    err = lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_Register, SYS_ERR_OK);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

// MEMSERV: Handle memory allocation requests
errval_t lmp_server_memory_alloc(struct lmp_chan *lc, size_t bytes, size_t align) {
    
    errval_t err = SYS_ERR_OK;
    
    // Checking for invalid allocation size or alignment
    if (bytes == 0 || align == 0) {
        debug_printf("size or alignment is zero\n");
        lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryAlloc, SYS_ERR_INVALID_SIZE);
        return SYS_ERR_INVALID_SIZE;
    }
    
    // TODO: Implement allocation policy for processes
    
    // Checking if requested allocation size is too big
//    if (bytes > MAX_ALLOCATION) {
//        debug_printf("requested size too big\n");
//        lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryAlloc, SYS_ERR_INVALID_SIZE);
//        return SYS_ERR_INVALID_SIZE;
//    }
    
    // Allocating ram capability with size bytes and alignment align
    struct capref ram;
    err = ram_alloc_aligned(&ram, bytes, align);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    // Responding by sending the ram capability back
    err = lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, ram, LMP_RequestType_MemoryAlloc, SYS_ERR_OK);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Deleting the ram capability
    cap_delete(ram);
    
    // Freeing the slot
    slot_free(ram);
    
    return err;
    
}

static ram_free_handler_t ram_free_handler;

// Registering ram_free_handler function
void register_ram_free_handler(ram_free_handler_t ram_free_function) {
    ram_free_handler = ram_free_function;
}

// MEMSERV: Handle requests to free memory
//  TODO: Test this!
errval_t lmp_server_memory_free(struct lmp_chan *lc, struct capref cap) {
    
    errval_t err = SYS_ERR_OK;
    
    // Freeing ram capability
    err = ram_free_handler(cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        err = lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryFree, MM_ERR_MM_FREE);
        return err;
    }

    // Responding that freeing ram capability was successful
    err = lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_MemoryFree, SYS_ERR_OK);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    return err;
    
}


// SPAWNSERV: Report all current PIDs
errval_t lmp_server_pid_discovery(struct lmp_chan *lc) {
    
    errval_t err;
    
    // Allocate memory for the response
    size_t ret_size;
    struct capref frame_cap;
    err = frame_alloc(&frame_cap, BASE_PAGE_SIZE, &ret_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Map it into memory
    uintptr_t *pids;
    err = paging_map_frame(get_current_paging_state(), (void **)&pids,
                           ret_size, frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Get all current PIDs
    size_t pid_count = get_all_pids(pids);
    
    // Send the ack and the number of PIDs
    err = lmp_chan_send2(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         LMP_RequestType_PidDiscover,
                         pid_count);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Send array of PIDs
    err = lmp_send_frame(lc, LMP_RequestType_StringLong, frame_cap, ret_size);
    
    //  TODO: figure out which request type to use for pid discover
    //err = lmp_send_frame(lc, LMP_RequestType_PidDiscover, frame_cap, ret_size);

    return err;
    
}

errval_t lmp_server_device_cap(struct lmp_chan *lc, lpaddr_t paddr, size_t bytes) {
    
    errval_t err;
    
    struct capref cap;
    
    err = slot_alloc(&cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    struct frame_identity id;
    invoke_frame_identify(cap_devices, &id);
    
    // Check if request is in range
    assert(id.base <= paddr && paddr + bytes <= id.base + id.bytes);
    
    // Calculating offset
    gensize_t offset = ((gensize_t) paddr) - id.base;
    
    err = cap_retype(cap, cap_devices, offset, ObjType_DevFrame, (gensize_t) bytes, 1);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
        
    // Send back retyped device capability (device frame)
    lmp_chan_send2(lc,
                   LMP_SEND_FLAGS_DEFAULT,
                   cap,
                   LMP_RequestType_DeviceCap,
                   err);
    
    // Delete capability
    cap_delete(cap);
    
    // Free slot
    slot_free(cap);
    
    return err;
    
}

// List Helper Functions

static collections_listnode *deregister_listeners;
static collections_listnode *dead_processes;

struct process_listener {
    domainid_t pid;
    struct lmp_chan *lc;
};


static int deregister_listeners_visit(void *data, void *arg) {
    struct process_listener *process_listener = (struct process_listener *) data;

    if (process_listener->pid == *(domainid_t *) arg) {
        lmp_chan_send1(process_listener->lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_ProcessDeregisterNotify);
    }
    return 0;
}

static int32_t deregister_listeners_remove(void *data, void *arg) {
    struct process_listener *process_listener = (struct process_listener *) data;
    return process_listener->pid == *(domainid_t *) arg;
}

void notify_deregister_listeners(domainid_t pid) {
    if (deregister_listeners == NULL || dead_processes == NULL) {
        collections_list_create(&deregister_listeners, free);
        collections_list_create(&dead_processes, free);
    }

    collections_list_visit(deregister_listeners, deregister_listeners_visit, &pid);
    collections_list_remove_if(deregister_listeners, deregister_listeners_remove, &pid);
}

static int dead_processes_find(void *data, void *arg) {
    return *(domainid_t *) data == *(domainid_t *) arg;
}

errval_t lmp_server_process_deregister(struct lmp_chan *lc) {
    if (deregister_listeners == NULL || dead_processes == NULL) {
        collections_list_create(&deregister_listeners, free);
        collections_list_create(&dead_processes, free);
    }

    domainid_t pid = process_pid_for_lmp_chan(lc);

    notify_deregister_listeners(pid);

    ump_send(&init_uc, &pid, sizeof(domainid_t), UMP_MessageType_DeregisterForward);

    return SYS_ERR_OK;
}

errval_t lmp_server_process_deregister_notify(struct lmp_chan *lc, domainid_t pid) {
    if (deregister_listeners == NULL || dead_processes == NULL) {
        collections_list_create(&deregister_listeners, free);
        collections_list_create(&dead_processes, free);
    }

    struct process_listener *process_listener = (struct process_listener *) malloc(sizeof(process_listener));
    process_listener->pid = pid;
    process_listener->lc = lc;


    if (collections_list_find_if(dead_processes, dead_processes_find, &pid)) {
        lmp_chan_send1(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_ProcessDeregisterNotify);
    } else {
        collections_list_insert(deregister_listeners, (void *) process_listener);
    }

    return SYS_ERR_OK;
}

/* MARK: - ========== Client ========== */

// Blocking call for receiving messages
void lmp_client_recv(struct lmp_chan *lc, struct capref *cap, struct lmp_recv_msg *msg) {


    // Use default waitset
    lmp_client_recv_waitset(lc, cap, msg, get_default_waitset());

}

// Blocking call for receiving messages on a specific waitset
void lmp_client_recv_waitset(struct lmp_chan *lc, struct capref *cap, struct lmp_recv_msg *msg, struct waitset *ws) {

    int done = 0;
    errval_t err;

    err = lmp_chan_register_recv(lc, ws, MKCLOSURE(lmp_client_wait, &done));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return;
    }
    
    while (!done) {
        event_dispatch(ws);
    }
    
    err = lmp_chan_recv(lc, msg, cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    #if PRINT_DEBUG
        debug_printf("Received LMP message with type %zu!\n", msg->words[0] & 0xFFFFFF);
    #endif

}
void lmp_client_wait(void *arg) {
    *(int *)arg = 1;
}


/* MARK: - ========== String ========== */

// Send a string on a specific channel (automatically select protocol)
errval_t lmp_send_string(struct lmp_chan *lc, const char *string) {
    
    errval_t err;
    
    // Get length of entire buffer (including '\0')
    size_t buf_len = strlen(string) + 1;
    
    // Check wether to use StringShort with send_short_buf() or StringLong with send_frame()
    if (buf_len <= sizeof(uintptr_t) * SHORT_BUF_SIZE) {
        
        return lmp_send_short_buf(lc, LMP_RequestType_StringShort, (void *) string, buf_len);
        
    }
    else {
        
        // Allocating frame capability
        size_t ret_size;
        struct capref frame_cap;
        err = frame_alloc(&frame_cap, buf_len, &ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Mapping frame into virtual address space
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               ret_size, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Copy string (including '\0') into memory/frame
        memcpy(buf, string, buf_len);
        
        // Send the frame to the recipient
        err = lmp_send_frame(lc, LMP_RequestType_StringLong, frame_cap, ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Cleaning up after sending
        err = paging_unmap(get_current_paging_state(), buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }

}

// Blocking call to receive a string on a channel (automatically select protocol)
errval_t lmp_recv_string(struct lmp_chan *lc, char **string) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for string receive
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_string_from_msg(lc, cap, msg.words, string);
}

// Process a string received through a message (automatically select protocol)
errval_t lmp_recv_string_from_msg(struct lmp_chan *lc, struct capref cap,
                                  uintptr_t *words, char **string) {
    
    errval_t err;
    
    // Assert that the message is valid
    assert(words[0] == LMP_RequestType_StringShort ||
           words[0] == LMP_RequestType_StringLong);
    
    if (words[0] == LMP_RequestType_StringShort) {
        
        size_t size;
        
        // Receive short buffer from message and copy it in newly allocated return argument string
        return lmp_recv_short_buf_from_msg(lc, LMP_RequestType_StringShort, words, (void **) string, &size);
    
    }
    else {
        
        // Process the message and get the capability and size
        struct capref frame_cap;
        size_t size;
        err = lmp_recv_frame_from_msg(lc, LMP_RequestType_StringLong, cap, words, &frame_cap, &size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Map the recieved frame into memory
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               size, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Get length of entire buffer (including '\0')
        size_t buf_len = strlen((char *) buf) + 1;
        
        // Allocate space to move return argument string onto the heap
        *string = malloc(buf_len);
        if (*string == NULL) {
            return LIB_ERR_MALLOC_FAIL;
        }
        
        // Copy the new string (including '\0')
        memcpy(*string, buf, buf_len);
        
        // Clean up the frame
        err = paging_unmap(get_current_paging_state(), buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, size);
         if (err_is_fail(err)) {
         debug_printf("%s\n", err_getstring(err));
         }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }
    
}


/* MARK: - ========== Buffer ========== */

// Send a buffer on a specific channel (automatically select protocol)
errval_t lmp_send_buffer(struct lmp_chan *lc, const void *buf,
                         size_t buf_len, uint8_t msg_type) {
    
    errval_t err;
    
    // Check wether to use BufferShort with send_short_buf() or BufferLong with send_frame()
    if (buf_len <= sizeof(uintptr_t) * SHORT_BUF_SIZE) {
        
        uintptr_t type = ((uintptr_t) msg_type) << 24;
        type |= LMP_RequestType_BufferShort;

        return lmp_send_short_buf_fast(lc,
                                       type,
                                       (void *) buf,
                                       buf_len);
        
    }
    else {

        // Allocating frame capability
        size_t ret_size;
        struct capref frame_cap;
        err = frame_alloc(&frame_cap, buf_len, &ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Mapping frame into virtual address space
        void *tx_buf;
        err = paging_map_frame(get_current_paging_state(), &tx_buf,
                               ret_size, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Copy buffer into memory/frame
        memcpy(tx_buf, buf, buf_len);
        
        // Send the frame to the recipient
        uintptr_t type = ((uintptr_t) msg_type) << 24;
        type |= LMP_RequestType_BufferLong;
        err = lmp_send_frame_fast(lc, type, frame_cap, buf_len);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Cleaning up after sending
        err = paging_unmap(get_current_paging_state(), tx_buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, ret_size);
         if (err_is_fail(err)) {
         debug_printf("%s\n", err_getstring(err));
         }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }
    
}

// Blocking call to receive a buffer on a channel (automatically select protocol)
errval_t lmp_recv_buffer(struct lmp_chan *lc, void **buf, size_t *len,
                         uint8_t *msg_type) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for string receive
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_buffer_from_msg(lc, cap, msg.words, buf, len, msg_type);
}

// Process a buffer received through a message (automatically select protocol)
errval_t lmp_recv_buffer_from_msg(struct lmp_chan *lc, struct capref cap,
                                  uintptr_t *words, void **buf, size_t *len,
                                  uint8_t *msg_type) {
    
    errval_t err;
    
    // Assert that the message is valid
    assert((words[0] & 0xFFFFFF) == LMP_RequestType_BufferShort ||
           (words[0] & 0xFFFFFF) == LMP_RequestType_BufferLong);
    
    // Extract msg_type
    *msg_type = (words[0] >> 24) & 0xFF;
    
    if ((words[0] & 0xFFFFFF) == LMP_RequestType_BufferShort) {
        
        // Receive short buffer from message and copy it in newly allocated return argument string
        return lmp_recv_short_buf_from_msg_fast(lc,
                                                LMP_RequestType_BufferShort,
                                                words,
                                                buf,
                                                len);
        
    }
    else {
        
        // Process the message and get the capability and size
        struct capref frame_cap;
        err = lmp_recv_frame_from_msg_fast(lc, LMP_RequestType_BufferLong, cap, words, &frame_cap, len);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Map the recieved frame into memory
        void *rx_buf;
        err = paging_map_frame(get_current_paging_state(), &rx_buf,
                               *len, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Allocate space to move return argument string onto the heap
        *buf = malloc(*len);
        if (*buf == NULL) {
            return LIB_ERR_MALLOC_FAIL;
        }
        
        // Copy the new string (including '\0')
        memcpy(*buf, rx_buf, *len);
        
        // Clean up the frame
        err = paging_unmap(get_current_paging_state(), rx_buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, size);
         if (err_is_fail(err)) {
         debug_printf("%s\n", err_getstring(err));
         }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }
    
}


/* MARK: - ========== Spawn ========== */

lmp_server_spawn_handler lmp_server_spawn_handler_func = NULL;
void lmp_server_spawn_register_handler(lmp_server_spawn_handler handler) {
    lmp_server_spawn_handler_func = handler;
}

errval_t lmp_send_spawn(struct lmp_chan *lc, const char *name, coreid_t core) {
    
    errval_t err;
    
    // Get length of the name
    size_t name_len = strlen(name);
    
    // Get length of entire buffer (including '\0')
    size_t buf_len = sizeof(coreid_t) + name_len + 1;
    
    // Check wether to use SpawnShort with send_short_buf() or SpawnLong with send_frame()
    if (buf_len <= sizeof(uintptr_t) * SHORT_BUF_SIZE) {
        
        // Construct buffer of length buf_len and fill in core id and name (including '\0')
        char buf[buf_len];
        
        // Copy core id into beginning of buffer
        *(coreid_t *) buf = core;
        
        // Initialize string as beginning of name
        char *string = ((char *) buf) + sizeof(coreid_t);

        // Copy name into buffer after core id
        memcpy(string, name, name_len + 1);
        
        // Send buffer using arguments
        return lmp_send_short_buf(lc, LMP_RequestType_SpawnShort, (void *) buf, buf_len);
        
    }
    else {
        
        // Allocating frame capability
        size_t ret_size;
        struct capref frame_cap;
        err = frame_alloc(&frame_cap, buf_len, &ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Mapping frame into virtual address space
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               ret_size, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Fill in core id and name string (including \0) into memory/frame
        
        // Copy core id into beginning of buffer
        *(coreid_t *) buf = core;
        
        // Initialize string as beginning of name
        char *string = ((char *) buf) + sizeof(coreid_t);
        
        // Copy name into buffer after core id
        memcpy(string, name, name_len + 1);
        
        // Send the frame to the recipient
        err = lmp_send_frame(lc, LMP_RequestType_SpawnLong, frame_cap, ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Cleaning up after sending
        err = paging_unmap(get_current_paging_state(), buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, ret_size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }

    return err;
    
}

// Blocking call to receive a spawn process name on a channel (automatically select protocol)
errval_t lmp_recv_spawn(struct lmp_chan *lc, char **name) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for spawn receive
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_spawn_from_msg(lc, cap, msg.words, name);
}

// Process a spawn received through a message (automatically select protocol)
errval_t lmp_recv_spawn_from_msg(struct lmp_chan *lc, struct capref cap,
                                 uintptr_t *words, char **name) {
    
    errval_t err;
    
    // Assert that the message is valid
    assert(words[0] == LMP_RequestType_SpawnShort ||
           words[0] == LMP_RequestType_SpawnLong);
    
    if (words[0] == LMP_RequestType_SpawnShort) {
        
        // Receive short buffer from message and copy it in newly allocated buffer
        void *buf;
        size_t size;
        lmp_recv_short_buf_from_msg(lc, LMP_RequestType_SpawnShort, words, &buf, &size);
        
        // Get core id out of buffer
        coreid_t core = *(coreid_t *) buf;
        
        // Initialize string as beginning of name
        char *string = ((char *) buf) + sizeof(coreid_t);
        
        // Get length of string
        size_t string_len = strlen(string);
        
        // Allocate space to move return argument name onto the heap
        *name = (char *) malloc(string_len + 1);
        if (*name == NULL) {
            return LIB_ERR_MALLOC_FAIL;
        }
        
        // Copy string (including '\0') from buffer into name
        memcpy(*name, string, string_len + 1);
        
        // Handle spawn request by passing it on to the spawn server
        domainid_t pid = 0;
        err = lmp_server_spawn_handler_func(*name, core, &pid);
        
        // Free buffer
        free(buf);
        
        // Send result to client
        lmp_chan_send3(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_SpawnShort, err, pid);
        
        return err;

    }
    else if (words[0] == LMP_RequestType_SpawnLong) {
        
        // Process the message and get the capability and size
        struct capref frame_cap;
        size_t size;
        err = lmp_recv_frame_from_msg(lc, LMP_RequestType_SpawnLong, cap, words, &frame_cap, &size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Map the recieved frame into memory
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               size, frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Extract core out of buffer
        coreid_t core = *(coreid_t *) buf;
        
        // Initialize string as beginning of name
        char *string = ((char *) buf) + sizeof(coreid_t);

        // Get length of string
        size_t string_len = strlen(string);
        
        // Allocate space to move return argument name onto the heap
        *name = malloc(string_len + 1);
        if (*name == NULL) {
            return LIB_ERR_MALLOC_FAIL;
        }
        
        // Copy the string (including '\0') from buffer to name
        memcpy(*name, string, string_len + 1);
        
        // Handle spawn request by passing it on to the spawn server
        domainid_t pid = 0;
        err = lmp_server_spawn_handler_func(*name, core, &pid);
        
        // Send result to client
        lmp_chan_send3(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_SpawnLong, err, pid);
        
        // Clean up the frame
        err = paging_unmap(get_current_paging_state(), buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        // FIXME: Make this work
        /*err = ram_free_handler(frame_cap, size);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }*/
        // Temporary less optimal soution:
        cap_delete(frame_cap);
        slot_free(frame_cap);
        
        return err;
        
    }
    
    return err;
    
}


/* MARK: - ========== Send buffer ==========  */

// Send a short buffer (using LMP arguments)
errval_t lmp_send_short_buf(struct lmp_chan *lc, uintptr_t type, void *buf, size_t size) {
    
    errval_t err;
    
    assert(size <= sizeof(uintptr_t) * SHORT_BUF_SIZE);
    
    // Allocate new memory to construct the arguments
    char *buf_arg = calloc(sizeof(uintptr_t), SHORT_BUF_SIZE);
    if (buf_arg == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Copy in the string
    memcpy(buf_arg, buf, size);
    
    // Send the LMP message
    err = lmp_chan_send9(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         type,
                         size,
                         ((uintptr_t *)buf_arg)[0],
                         ((uintptr_t *)buf_arg)[1],
                         ((uintptr_t *)buf_arg)[2],
                         ((uintptr_t *)buf_arg)[3],
                         ((uintptr_t *)buf_arg)[4],
                         ((uintptr_t *)buf_arg)[5],
                         ((uintptr_t *)buf_arg)[6]);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        free(buf_arg);
        return err;
    }
    
    // Free the memory for constructing the arguments
    free(buf_arg);
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive the status code form recipient
    lmp_client_recv(lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == (type & 0xFFFFFF));
    
    // Return an error if things didn't work
    if (err_is_fail(msg.words[1])) {
        return msg.words[1];
    }
    
    // Return the status code
    return msg.words[2] == size ? SYS_ERR_OK : -1;
    
}

// Send an entire frame capability
errval_t lmp_send_frame(struct lmp_chan *lc, uintptr_t type, struct capref frame_cap, size_t frame_size) {
    
    errval_t err;
    
    // Sending frame capability and it's size
    err = lmp_chan_send2(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         frame_cap,
                         type,
                         frame_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Receive ack from recipient
    lmp_client_recv(lc, &cap, &msg);
    
    // Check we actually got a valid response
    assert(msg.words[0] == (type & 0xFFFFFF));
    
    // Return an error if things didn't work
    if (err_is_fail(msg.words[1])) {
        return msg.words[1];
    }
    
    // Return the status code
    return msg.words[2] == frame_size ? SYS_ERR_OK : -1;
}


/* MARK: - ========== Receive buffer ==========  */

// Receive a short buffer on a channel (using LMP arguments)
errval_t lmp_recv_short_buf(struct lmp_chan *lc, enum lmp_request_type type, void **buf, size_t *size) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for short buffer receive
    lmp_client_recv(lc, &cap, &msg);
    
    // Receive short buffer from message and copy it in newly allocated buffer
    return lmp_recv_short_buf_from_msg(lc, type, msg.words, buf, size);
}

// Process a short buffer received through a message (using LMP arguments)
errval_t lmp_recv_short_buf_from_msg(struct lmp_chan *lc, enum lmp_request_type type, uintptr_t *words,
                                     void **buf, size_t *size) {
    
    errval_t err;
    
    // Assert this is a valid message
    assert((words[0] & 0xFFFFFF) == type);
    
    // Get the length of the buffer
    *size = words[1];
    
    // Allocate new space for the received string
    *buf = malloc(*size + 1);
    if (*buf == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Copy words from message to buffer (size includes '\0')
    memcpy(*buf, words + 2, *size);
    
    // Send a confirmation
    err = lmp_chan_send3(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         type,
                         SYS_ERR_OK,
                         *size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    return err;
    
}

// Receive a frame on a channel
errval_t lmp_recv_frame(struct lmp_chan *lc, enum lmp_request_type type, struct capref *frame_cap, size_t *size) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for frame receive
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_frame_from_msg(lc, type, cap, msg.words, frame_cap, size);
}

// Process a frame received through a message
errval_t lmp_recv_frame_from_msg(struct lmp_chan *lc, enum lmp_request_type type, struct capref msg_cap,
                                 uintptr_t *words, struct capref *frame_cap, size_t *size) {
    
    errval_t err;
    
    // Assert this is a valid message
    assert((words[0] & 0xFFFFFF) == type);
    
    // Get the size of the received frame
    *size = words[1];
    
    // Pass on the capref
    memcpy(frame_cap, &msg_cap, sizeof(struct capref));
    
    // Make a new slot available for the next incoming capability
    err = lmp_chan_alloc_recv_slot(lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Send a confirmation
    err = lmp_chan_send3(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         type,
                         SYS_ERR_OK,
                         *size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    return err;
    
}



/* MARK: - ========== Send buffer (FAST) ==========  */

// Send a short buffer (using LMP arguments)
errval_t lmp_send_short_buf_fast(struct lmp_chan *lc, uintptr_t type, void *buf, size_t size) {

    errval_t err;

    assert(size <= sizeof(uintptr_t) * SHORT_BUF_SIZE);

    // Allocate new memory to construct the arguments
    char *buf_arg = calloc(sizeof(uintptr_t), SHORT_BUF_SIZE);
    if (buf_arg == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }

    // Copy in the string
    memcpy(buf_arg, buf, size);

    // Send the LMP message
    err = lmp_chan_send9(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         NULL_CAP,
                         type,
                         size,
                         ((uintptr_t *)buf_arg)[0],
                         ((uintptr_t *)buf_arg)[1],
                         ((uintptr_t *)buf_arg)[2],
                         ((uintptr_t *)buf_arg)[3],
                         ((uintptr_t *)buf_arg)[4],
                         ((uintptr_t *)buf_arg)[5],
                         ((uintptr_t *)buf_arg)[6]);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        free(buf_arg);
        return err;
    }

    // Free the memory for constructing the arguments
    free(buf_arg);
    
    return SYS_ERR_OK;
    
}

// Send an entire frame capability
errval_t lmp_send_frame_fast(struct lmp_chan *lc, uintptr_t type, struct capref frame_cap, size_t frame_size) {
    
    errval_t err;
    
    // Sending frame capability and it's size
    err = lmp_chan_send2(lc,
                         LMP_SEND_FLAGS_DEFAULT,
                         frame_cap,
                         type,
                         frame_size);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Return the status code
    return SYS_ERR_OK;
}


/* MARK: - ========== Receive buffer (FAST) ==========  */

// Receive a short buffer on a channel (using LMP arguments)
errval_t lmp_recv_short_buf_fast(struct lmp_chan *lc, enum lmp_request_type type, void **buf, size_t *size) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for short buffer receive
    lmp_client_recv(lc, &cap, &msg);
    
    // Receive short buffer from message and copy it in newly allocated buffer
    return lmp_recv_short_buf_from_msg_fast(lc, type, msg.words, buf, size);
}

// Process a short buffer received through a message (using LMP arguments)
errval_t lmp_recv_short_buf_from_msg_fast(struct lmp_chan *lc, enum lmp_request_type type, uintptr_t *words,
                                     void **buf, size_t *size) {
    
    // Assert this is a valid message
    assert((words[0] & 0xFFFFFF) == type);
    
    // Get the length of the buffer
    *size = words[1];
    
    // Allocate new space for the received string
    *buf = malloc(*size + 1);
    if (*buf == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Copy words from message to buffer (size includes '\0')
    memcpy(*buf, words + 2, *size);
    
    return SYS_ERR_OK;
    
}

// Receive a frame on a channel
errval_t lmp_recv_frame_fast(struct lmp_chan *lc, enum lmp_request_type type, struct capref *frame_cap, size_t *size) {
    
    // Initialize capref and message
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    // Wait for frame receive
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_frame_from_msg_fast(lc, type, cap, msg.words, frame_cap, size);
}

// Process a frame received through a message
errval_t lmp_recv_frame_from_msg_fast(struct lmp_chan *lc, enum lmp_request_type type, struct capref msg_cap,
                                 uintptr_t *words, struct capref *frame_cap, size_t *size) {
    
    errval_t err;
    
    // Assert this is a valid message
    assert((words[0] & 0xFFFFFF) == type);
    
    // Get the size of the received frame
    *size = words[1];
    
    // Pass on the capref
    memcpy(frame_cap, &msg_cap, sizeof(struct capref));
    
    // Make a new slot available for the next incoming capability
    err = lmp_chan_alloc_recv_slot(lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    return err;
    
}

