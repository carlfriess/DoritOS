#include <stdio.h>
#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/lmp.h>
#include <aos/process.h>

#define MAX_ALLOCATION 100000000

#define PRINT_DEBUG 0

/* ========== Server ========== */
void lmp_server_dispatcher(void *arg) {

#if PRINT_DEBUG
    debug_printf("LMP Message Received!\n");
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
    domainid_t *ret_list;
    size_t pid_count;

    // Check message type and handle
    switch (msg.words[0]) {
            
            
        case LMP_RequestType_Number:
#if PRINT_DEBUG
            debug_printf("Number Message!\n");
#endif
            lmp_chan_send2(lc,
                           LMP_SEND_FLAGS_DEFAULT,
                           NULL_CAP,
                           LMP_RequestType_Number,
                           msg.words[1]);
            break;
            
            
        case LMP_RequestType_StringShort:
        case LMP_RequestType_StringLong:
    #if PRINT_DEBUG
                debug_printf("String Message!\n");
    #endif
            lmp_recv_string_from_msg(lc, cap, msg.words, &string);
            printf("Received string: %s\n", string);
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
            
            
        case LMP_RequestType_Spawn:
#if PRINT_DEBUG
            debug_printf("Spawn Message!\n");
#endif
            lmp_server_spawn(lc, msg.words);
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
            // Get all current PIDs
            pid_count = get_all_pids(&ret_list);
            
            // Send the ack and the number of PIDs
            lmp_chan_send2(lc,
                           LMP_SEND_FLAGS_DEFAULT,
                           NULL_CAP,
                           LMP_RequestType_PidDiscover,
                           pid_count);
            
            // Send array of PIDs
            lmp_send_string(lc, (char *)ret_list);
            
            break;
            
            
        case LMP_RequestType_TerminalGetChar:
#if PRINT_DEBUG
            debug_printf("Terminal Put Message!\n");
#endif
            lmp_server_terminal_getchar(lc);
            break;
        case LMP_RequestType_TerminalPutChar:
#if PRINT_DEBUG
            debug_printf("Terminal Message!\n");
#endif
            lmp_server_terminal_putchar(lc, msg.words[1]);
            break;

            
        default:
#if PRINT_DEBUG
            debug_printf("Invalid Message!\n");
#endif
            break;
            
            
    }


    err = lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_server_dispatcher, (void *) lc));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

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

// TODO: Test this!
errval_t lmp_server_memory_free(struct lmp_chan *lc, struct capref cap, size_t bytes) {
    
    errval_t err = SYS_ERR_OK;
    
    // Freeing ram capability
    err = ram_free_handler(cap, bytes);
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

static lmp_server_spawn_handler lmp_server_spawn_handler_func = NULL;

void lmp_server_spawn_register_handler(lmp_server_spawn_handler handler) {
    lmp_server_spawn_handler_func = handler;
}

void lmp_server_spawn(struct lmp_chan *lc, uintptr_t *args) {

    errval_t err;
    domainid_t pid = 0;
    
    err = lmp_server_spawn_handler_func((char *)(args+2), (coreid_t) args[1], &pid);
    
    // Send result to client
    lmp_chan_send3(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_Spawn, err, pid);
    
}

void lmp_server_terminal_getchar(struct lmp_chan *lc) {
    errval_t err = SYS_ERR_OK;
    char c = '\0';
    while (c == '\0') {
        sys_getchar(&c);
    }

    lmp_chan_send3(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_TerminalGetChar, err, c);
}

void lmp_server_terminal_putchar(struct lmp_chan *lc, char c) {
    errval_t err = SYS_ERR_OK;
    sys_print(&c, sizeof(char));

    lmp_chan_send3(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_RequestType_TerminalGetChar, err, c);
}

/* ========== Client ========== */

void lmp_client_recv(struct lmp_chan *arg, struct capref *cap, struct lmp_recv_msg *msg) {
    int done = 0;
    errval_t err;

    struct lmp_chan *lc = (struct lmp_chan *) arg;

    err = lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_client_wait, &done));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return;
    }

    while (!done) {
        event_dispatch(get_default_waitset());
    }

    err = lmp_chan_recv(lc, msg, cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
}

void lmp_client_wait(void *arg) {
    *(int *)arg = 1;
}

/* ========== Aux ========== */

errval_t lmp_send_string(struct lmp_chan *lc, const char *string) {
    
    errval_t err;
    
    // Get length of the string
    size_t len = strlen(string);
    
    // Check wether to use StringShort or StringLong protocol
    if (len < sizeof(uintptr_t) * 8) {
        
        /* StringShort */
        
        // Allocate new memory to construct the arguments
        char *string_arg = calloc(sizeof(uintptr_t), 8);
        
        // Copy in the string
        memcpy(string_arg, string, len);
        
        // Send the LMP message
        err = lmp_chan_send9(lc,
                             LMP_SEND_FLAGS_DEFAULT,
                             NULL_CAP,
                             LMP_RequestType_StringShort,
                             ((uintptr_t *)string_arg)[0],
                             ((uintptr_t *)string_arg)[1],
                             ((uintptr_t *)string_arg)[2],
                             ((uintptr_t *)string_arg)[3],
                             ((uintptr_t *)string_arg)[4],
                             ((uintptr_t *)string_arg)[5],
                             ((uintptr_t *)string_arg)[6],
                             ((uintptr_t *)string_arg)[7]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            free(string_arg);
            return err;
        }
        
        // Free the memory for constructing the arguments
        free(string_arg);
        
        // Receive the status code form recipient
        struct capref cap;
        struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
        lmp_client_recv(lc, &cap, &msg);
        
        // Check we actually got a valid response
        assert(msg.words[0] == LMP_RequestType_StringShort);
        
        // Return an error if things didn't work
        if (err_is_fail(msg.words[1])) {
            return msg.words[1];
        }
        
        // Return the status code
        return msg.words[2] == len ? SYS_ERR_OK : -1;
        
    }
    else {
        
        /* StringLong */
        
        // Allocating frame capability
        size_t retbytes;
        struct capref frame;
        err = frame_alloc(&frame, len + 1, &retbytes);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Mapping frame into virtual address space
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               retbytes, frame, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Copy string into memory
        memcpy(buf, string, len);
        *((char *)buf + len) = '\0';
        
        // Sending frame capability and size where string is stored
        err = lmp_chan_send2(lc, LMP_SEND_FLAGS_DEFAULT, frame, LMP_RequestType_StringLong, retbytes);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }

        // Cleaning up after sending
        cap_delete(frame);
        slot_free(frame);
        
        // Receive ack from recipient
        struct capref cap;
        struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
        lmp_client_recv(lc, &cap, &msg);
        
        // Return an error if things didn't work
        if (err_is_fail(msg.words[1])) {
            return msg.words[1];
        }
        
        // Return the status code
        return msg.words[2] == len ? SYS_ERR_OK : -1;
        
    }
    
}

// Receive a string on a channel
errval_t lmp_recv_string(struct lmp_chan *lc, char **string) {
    
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
    
    lmp_client_recv(lc, &cap, &msg);
    
    return lmp_recv_string_from_msg(lc, cap, msg.words, string);
    
}

errval_t lmp_recv_string_from_msg(struct lmp_chan *lc, struct capref cap,
                                  uintptr_t *words, char **string) {
    
    errval_t err = SYS_ERR_OK;
    
    // Check which string protocol was received
    if (words[0] == LMP_RequestType_StringShort) {
    
        // Get the length of the received string
        size_t len = strlen((char *)(words+1));
        
        // Sanity check
        assert(len < sizeof(uintptr_t) * 8);
    
        // Allocate new space for the received string
        *string = malloc(len + 1);
        
        // Copy in the new string
        memcpy(*string, words+1, len);
        *(*string + len) = '\0';

        // Send a confirmation
        err = lmp_chan_send3(lc,
                             LMP_SEND_FLAGS_DEFAULT,
                             NULL_CAP,
                             LMP_RequestType_StringShort,
                             SYS_ERR_OK,
                             len);
        
        return err;
        
    }
    else {
        
        // Mapping the received capability
        void *buf;
        err = paging_map_frame(get_current_paging_state(), &buf,
                               words[1], cap, NULL, NULL);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Get the length of the received string
        size_t len = strlen((char *)buf);
        
        // Allocate new space for the received string
        *string = malloc(len + 1);
        
        // Copy in the new string
        memcpy(*string, buf, len);
        *(*string + len) = '\0';
        
        err = lmp_chan_alloc_recv_slot(lc);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }
        
        // Send a confirmation
        lmp_chan_send3(lc,
                       LMP_SEND_FLAGS_DEFAULT,
                       NULL_CAP,
                       LMP_RequestType_StringLong,
                       SYS_ERR_OK,
                       len);
        
        // Clean up the frame
        err = paging_unmap(get_current_paging_state(), buf);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        err = ram_free_handler(cap, words[1]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
        }
        
        return err;
        
    }
    
}

