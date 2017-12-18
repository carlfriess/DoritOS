//
//  urpc.c
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <aos/aos.h>
#include <aos/aos_rpc.h>
#include <aos/lmp.h>
#include <aos/urpc_protocol.h>
#include <spawn_serv.h>


#include "aos/urpc.h"

extern struct ump_chan init_uc;
extern lmp_server_spawn_handler lmp_server_spawn_handler_func;

// MARK: - Init URPC Server

static void urpc_spawn_handler(struct ump_chan *chan, void *msg, size_t size,
                               ump_msg_type_t msg_type);
static void urpc_get_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type);
static void urpc_put_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type);

// Handler for init URPC server:
void urpc_init_server_handler(struct ump_chan *chan, void *msg, size_t size,
                              ump_msg_type_t msg_type) {
    
    switch (msg_type) {
            
        case UMP_MessageType_Spawn:
            urpc_spawn_handler(chan, msg, size, msg_type);
            break;
            
        case UMP_MessageType_TerminalGetChar:
            urpc_get_char_handler(chan, msg, size, msg_type);
            break;

        case UMP_MessageType_TerminalPutChar:
            urpc_put_char_handler(chan, msg, size, msg_type);
            break;
            
        case UMP_MessageType_RegisterProcess:
            urpc_register_process_handler(chan, msg, size, msg_type);
            break;
            
        case UMP_MessageType_UrpcBindRequest:
            urpc_handle_ump_bind_request(chan, msg, size, msg_type);
            break;
            
        default:
            USER_PANIC("Unknown UMP message type\n");
            break;
            
    }
    
}

// Handle UMP_MessageType_Spawn
static void urpc_spawn_handler(struct ump_chan *chan, void *msg, size_t size,
                               ump_msg_type_t msg_type) {
    
    struct urpc_spaw_response res;
    
    // Pass message to spawn server
    res.err = lmp_server_spawn_handler_func((char *) msg, disp_get_core_id(), &res.pid);
    
    // Send response back to requesting core
    ump_send(chan,
             (void *) &res,
             sizeof(struct urpc_spaw_response),
             UMP_MessageType_SpawnAck);
    
}

// Handle UMP_MessageType_TerminalGetChar
static void urpc_get_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type) {
    char c;
    do {
        sys_getchar(&c);
    } while (c == '\0');
    
    ump_send(chan, &c, sizeof(char), UMP_MessageType_TerminalGetCharAck);
    
}

// Handle UMP_MessageType_TerminalPutChar

static void urpc_put_char_handler(struct ump_chan *chan, void *msg, size_t size,
                                    ump_msg_type_t msg_type) {
    // Print character
    sys_print((char *) msg, sizeof(char));

    // Send acknowledgement
    ump_send(&init_uc, msg, sizeof(char), UMP_MessageType_TerminalPutCharAck);
}

// Handle UMP_MessageType_RegisterProcess
void urpc_register_process_handler(struct ump_chan *chan, void *msg,
                                   size_t size, ump_msg_type_t msg_type) {
    
    struct urpc_process_register *req = msg;
    struct process_info *new_process = malloc(sizeof(struct process_info));
    assert(new_process != NULL);
    
    new_process->core_id = req->core_id;
    new_process->pid = req->pid;
    new_process->name = (char *) malloc(strlen(req->name));
    assert(new_process->name != NULL);
    strcpy(new_process->name, req->name);
    
    process_register(new_process);
    
    ump_send_one(chan,
                 &new_process->pid,
                 sizeof(domainid_t),
                 UMP_MessageType_RegisterProcessAck,
                 1);
    
}



// MARK: - Init URPC Client

// RPC for registering a process
void urpc_process_register(struct process_info *pi) {
    
    size_t msg_size = sizeof(struct urpc_process_register) + strlen(pi->name) + 1;
    struct urpc_process_register *msg = (struct urpc_process_register *) malloc(msg_size);
    assert(msg != NULL);
    
    msg->core_id = pi->core_id;
    msg->pid = pi->pid;
    strcpy(msg->name, pi->name);
    
    ump_send(&init_uc, (void *)msg, msg_size, UMP_MessageType_RegisterProcess);
    
    free(msg);
    
    domainid_t *pid = malloc(UMP_SLOT_DATA_BYTES);
    
    ump_msg_type_t msg_type;
    uint8_t last;
    errval_t err;
    do {
        err = ump_recv_one(&init_uc, (void *)pid, &msg_type, &last);
    } while (err == LIB_ERR_NO_UMP_MSG);
    assert(err_is_ok(err));
    assert(msg_type == UMP_MessageType_RegisterProcessAck);
    assert(last == 1);
    
    pi->pid = *pid;
    
    free(pid);
    
}



// MARK: - Generic Server

static struct lmp_chan *get_init_lmp_chan(void) {
    return get_init_rpc()->lc;
}

// Accept a bind request and set up the URPC channel
errval_t urpc_accept(struct urpc_chan *chan) {
    
    errval_t err;
    
    // Get the channel to this core's init
    struct lmp_chan *lc = get_init_lmp_chan();
    
    // Initialize capref and message
    struct capref ump_frame_cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    // Wait for and receive a binding request
    lmp_client_recv(lc, &ump_frame_cap, &msg);
    
    // Check we received a correct message
    assert(msg.words[0] == LMP_RequestType_UmpBind ||
           msg.words[0] == LMP_RequestType_LmpBind);
    
    // Make a new slot available for the next incoming capability
    err = lmp_chan_alloc_recv_slot(lc);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Set the new chanel to correct transport protocol
    chan->use_lmp = msg.words[0] == LMP_RequestType_LmpBind;
    
    // Switch between transport protocols
    if (chan->use_lmp) {
        
        // MARK: LMP
        
        // Allocate lmp channel
        chan->lmp = (struct lmp_chan *) malloc(sizeof(struct lmp_chan));
        assert(chan->lmp);
        
        // Open channel to messages
        err = lmp_chan_accept(chan->lmp, LMP_RECV_LENGTH, ump_frame_cap);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Allocate recv slot
        err = lmp_chan_alloc_recv_slot(chan->lmp);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Send lmp endpoint to client via init
        err = lmp_chan_send2(lc,
                             LMP_SEND_FLAGS_DEFAULT,
                             chan->lmp->local_cap,
                             LMP_RequestType_LmpBind,
                             msg.words[1]);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Initialize capref and message
        struct capref cap;
        struct lmp_recv_msg msg1 = LMP_RECV_MSG_INIT;
        
        // Wait on response from client
        lmp_client_recv(chan->lmp, &cap, &msg1);
        
        // Check we received a valid response
        assert(msg.words[0] == LMP_RequestType_LmpBind);
        
    }
    else {
        
        // MARK: UMP
        
        // Allocate ump channel
        chan->ump = (struct ump_chan *) malloc(sizeof(struct ump_chan));
        assert(chan->ump);
    
        // Initialize the UMP channel
        ump_chan_init(chan->ump, UMP_SERVER_BUF_SELECT);
        
        // Get the UMP frame identity
        err = frame_identify(ump_frame_cap, &chan->ump->fi);
        if (err_is_fail(err)) {
            return err;
        }
        
        // Map the received UMP frame
        err = paging_map_frame(get_current_paging_state(),
                               (void **) &chan->ump->buf,
                               chan->ump->fi.bytes,
                               ump_frame_cap,
                               NULL,
                               NULL);
        if (err_is_fail(err)) {
            return err;
        }
        
    }
    
    // Send the ack over URPC
    char buf[] = "Hi there!";
    err = urpc_send(chan, (void *) buf, sizeof(buf),
                    URPC_MessageType_UrpcBindAck);
    
    return err;
    
}



// MARK: - Generic Client

// Bind to a URPC server with a specific PID
errval_t urpc_bind(domainid_t pid, struct urpc_chan *chan, bool use_lmp) {
    
    errval_t err;
    
    // Set the new chanel to correct transport protocol
    chan->use_lmp = use_lmp;
    
    // Get the channel to this core's init
    struct lmp_chan *lc = get_init_lmp_chan();
    
    // Switch between transport protocols
    if (chan->use_lmp) {
        
        // MARK: LMP
        
        // Allocate lmp channel
        chan->lmp = (struct lmp_chan *) malloc(sizeof(struct lmp_chan));
        assert(chan->lmp);
        
        // Open channel to messages
        err = lmp_chan_accept(chan->lmp, LMP_RECV_LENGTH, NULL_CAP);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Allocate recv slot
        err = lmp_chan_alloc_recv_slot(chan->lmp);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Send lmp endpoint to server via init
        err = lmp_chan_send2(lc,
                             LMP_SEND_FLAGS_DEFAULT,
                             chan->lmp->local_cap,
                             LMP_RequestType_LmpBind,
                             pid);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Initialize capref and message
        struct capref cap;
        struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;
        
        // Wait on response from server
        lmp_client_recv(lc, &cap, &msg);
        
        // Check we received a valid response
        assert(msg.words[0] == LMP_RequestType_LmpBind);
        assert(msg.words[1] == pid);
        
        // Set the remote endpoint
        chan->lmp->remote_cap = cap;
        
        // Send an ack to the server
        err = lmp_chan_send1(chan->lmp,
                             LMP_SEND_FLAGS_DEFAULT,
                             NULL_CAP,
                             LMP_RequestType_LmpBind);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
    }
    else {
        
        // MARK: UMP
        
        // Allocate ump channel
        chan->ump = (struct ump_chan *) malloc(sizeof(struct ump_chan));
        assert(chan->ump);
        
        // Allocate a frame for the new UMP channel
        struct capref ump_frame_cap;
        size_t ump_frame_size = UMP_BUF_SIZE;
        err = frame_alloc(&ump_frame_cap, ump_frame_size, &ump_frame_size);
        assert(ump_frame_size >= UMP_BUF_SIZE);
        
        // Send a bind request with the frame capability to this core's init
        lmp_chan_send2(lc,
                       LMP_SEND_FLAGS_DEFAULT,
                       ump_frame_cap,
                       LMP_RequestType_UmpBind,
                       pid);
        
        // Initialize the UMP channel
        ump_chan_init(chan->ump, UMP_CLIENT_BUF_SELECT);
        
        // Map the allocated UMP frame
        err = paging_map_frame(get_current_paging_state(),
                               (void **) &chan->ump->buf, ump_frame_size,
                               ump_frame_cap, NULL, NULL);
        if (err_is_fail(err)) {
            return err;
        }
        
        // Get the frame identity
        err = frame_identify(ump_frame_cap, &chan->ump->fi);
        if (err_is_fail(err)) {
            return err;
        }
        
    }
    
    // Wait for ack from server
    char *retmsg;
    size_t retsize;
    urpc_msg_type_t msg_type;
    err = urpc_recv_blocking(chan, (void **) &retmsg, &retsize, &msg_type);
    assert(err_is_ok(err));
        
    // Check we recieved the correct message
    assert(msg_type == URPC_MessageType_UrpcBindAck);
    
    debug_printf("BIND: Received \"%s\" from server.\n", retmsg);
    
    return SYS_ERR_OK;
    
}


// MARK: - Generic Send & Receive

// Send on a URPC channel
errval_t urpc_send(struct urpc_chan *chan, void *buf, size_t size,
                   urpc_msg_type_t msg_type) {
    
    // Switch between transport protocols
    if (chan->use_lmp) {

        return lmp_send_buffer(chan->lmp, buf, size, msg_type);
        
    }
    else {
        
        // MARK: UMP
        
        ump_send(chan->ump, buf, size, (ump_msg_type_t) msg_type);
        return SYS_ERR_OK;
        
    }
    
}

// Receive on a URPC channel
errval_t urpc_recv(struct urpc_chan *chan, void **buf, size_t *size,
                   urpc_msg_type_t* msg_type) {
    
    // Switch between transport protocols
    if (chan->use_lmp) {
        
        // MARK: LMP
        
        return lmp_recv_buffer(chan->lmp, buf, size, msg_type);
        
    }
    else {
        
        // MARK: UMP
        
        errval_t err = ump_recv(chan->ump, buf, size,
                                (ump_msg_type_t *) msg_type);
        if (err == LIB_ERR_NO_UMP_MSG) {
            return LIB_ERR_NO_URPC_MSG;
        }
        return err;
        
    }
    
}

// Blockingly receive on a URPC channel
errval_t urpc_recv_blocking(struct urpc_chan *chan, void **buf, size_t *size,
                            urpc_msg_type_t* msg_type) {
    
    // Switch between transport protocols
    if (chan->use_lmp) {
                
        // MARK: LMP
        
        return lmp_recv_buffer(chan->lmp, buf, size, msg_type);
        
    }
    else {
        
        // MARK: UMP
        
        ump_recv_blocking(chan->ump, buf, size, (ump_msg_type_t *) msg_type);
        return SYS_ERR_OK;
        
    }
    
}



// MARK: - URPC bind handlers

// Forward a URPC bind request over UMP to the other core
static void urpc_forward_request_ump(struct capref ump_frame_cap, domainid_t pid) {
    
    errval_t err;
    
    // Structured message for UMP request
    struct urpc_bind_request req;
    req.pid = pid;
    
    // Get the frame identity of the UMP frame
    err = frame_identify(ump_frame_cap, &req.fi);
    if (err_is_fail(err)) {
        debug_printf("Failed to identify UMP frame: %s\n", err_getstring(err));
        return;
    }
    
    // Send the UMP message to the other core
    err = ump_send(&init_uc,
                   &req,
                   sizeof(struct urpc_bind_request),
                   UMP_MessageType_UrpcBindRequest);
    if (err_is_fail(err)) {
        debug_printf("Failed sending UMP message: %s\n", err_getstring(err));
        return;
    }
    
}

// Forward a URPC bind request over LMP to the server process
static void urpc_forward_request_lmp(struct lmp_chan *lc,
                                     struct capref ump_frame_cap,
                                     enum lmp_request_type type,
                                     domainid_t src) {
    
    // Send a bind request with the frame capability to the server process
    lmp_chan_send2(lc,
                   LMP_SEND_FLAGS_DEFAULT,
                   ump_frame_cap,
                   type,
                   src);
    
}

// Handle a URPC bind request received via LMP
void urpc_handle_lmp_bind_request(struct lmp_chan *lc,
                                  struct capref msg_cap,
                                  struct lmp_recv_msg msg) {
    
    // Sanity check: cannot bind to init
    assert(msg.words[1] != 0 && "Cannot bind to init!");
    
    // Get the requested process by it's PID
    struct process_info *pi = process_info_for_pid(msg.words[1]);
    
    // Check the process exists
    if (pi == NULL) {
        return;
    }
    
    // Check if the process is on this core
    if (disp_get_core_id() == pi->core_id) {
        
        // Forward the request to the process
        urpc_forward_request_lmp(pi->lc,
                                 msg_cap,
                                 msg.words[0],
                                 process_pid_for_lmp_chan(lc));
        
    }
    else {
        
        // Cannot use LMP binding between cores!
        assert(msg.words[0] == LMP_RequestType_UmpBind);
        
        // Forward the request to the other core
        urpc_forward_request_ump(msg_cap, pi->pid);
        
    }
    
}

// Handle a URPC bind request received via UMP
void urpc_handle_ump_bind_request(struct ump_chan *chan, void *msg, size_t size,
                                  ump_msg_type_t msg_type) {
    
    errval_t err;
    
    // Sanity check: make sure we have a correct message
    assert(msg_type == UMP_MessageType_UrpcBindRequest);
    
    // Get the request
    struct urpc_bind_request *req = (struct urpc_bind_request *) msg;
    
    // Get the requested process by it's PID
    struct process_info *pi = process_info_for_pid(req->pid);
    
    // Check the process exists
    if (pi == NULL) {
        return;
    }
    
    // Sanity check: make sure we are on the correct core
    assert(disp_get_core_id() == pi->core_id);
    
    // Allocate a slot for the UMP frame
    struct capref ump_frame_cap;
    err = slot_alloc(&ump_frame_cap);
    if (err_is_fail(err)) {
        debug_printf("Slot allocation for UMP frame failed: %s\n", err_getstring(err));
        return;
    }
    
    // Forge the frame cap for the UMP frame
    err = frame_forge(ump_frame_cap,
                      req->fi.base,
                      req->fi.bytes,
                      disp_get_core_id());
    if (err_is_fail(err)) {
        debug_printf("Frame forge for UMP frame failed: %s\n", err_getstring(err));
        return;
    }
    
    // Forward the request to the correct process
    urpc_forward_request_lmp(pi->lc, ump_frame_cap, LMP_RequestType_UmpBind, 0);
    
}
