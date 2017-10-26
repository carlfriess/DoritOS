#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/lmp.h>

/*
 * LMP Protocol Definitions
 *
 * ==== Register ====
 *
 * arg0: LMP_MessageType_Register
 *
 * ==== Memory ====
 *
 * arg0: LMP_MessageType_Memory
 * arg1: size_t align
 * arg2: size_t bytes
 *
 * ==== Spawn ====
 *
 * arg0: LMP_MessageType_Spawn
 *
 * ==== Terminal ====
 *
 * arg0: LMP_MessageType_Terminal
 *
 */

enum lmp_request_type {
    LMP_RequestType_NULL = 0,
    LMP_RequestType_Register,
    LMP_RequestType_Memory,
    LMP_RequestType_Spawn,
    LMP_RequestType_Terminal
};

enum lmp_response_type {
    LMP_ResponseType_NULL = 0,
    LMP_ResponseType_Ok,
    LMP_ResponseType_Error
};


/* ========== Server ========== */
void lmp_server_dispatcher(void *arg) {

    debug_printf("LMP Message Received!\n");

    errval_t err;

    struct lmp_chan *lc = (struct lmp_chan *) arg;
    struct capref cap;
    struct lmp_recv_msg msg = LMP_RECV_MSG_INIT;

    err = lmp_chan_recv(lc, &msg, &cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }


    // Check message type and handle
    switch (msg.words[0]) {
        case LMP_RequestType_Register:
            debug_printf("Registration Message!\n", msg.words[0]);
            lmp_server_register(lc, cap);
            break;
        case LMP_RequestType_Memory:
            debug_printf("Memory Message!\n", msg.words[0]);

            break;
        case LMP_RequestType_Spawn:
            debug_printf("Spawn Message!\n", msg.words[0]);

            break;
        case LMP_RequestType_Terminal:
            debug_printf("Terminal Message!\n", msg.words[0]);

            break;
        default:
            debug_printf("Invalid Message!\n", msg.words[0]);
    }


    err = lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_server_dispatcher, (void *) lc));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

void lmp_server_register(struct lmp_chan *lc, struct capref cap) {
    errval_t err;
    debug_printf("LMP handler registered!\n");

    char str[100];
    debug_print_cap_at_capref(str, 100, cap);
    debug_printf("%s\n", str);

    lc->remote_cap = cap;
//    err = lmp_chan_alloc_recv_slot(lc);
//    if (err_is_fail(err)) {
//        debug_printf("%s\n", err_getstring(err));
//    }

    debug_print_cap_at_capref(str, 100, lc->remote_cap);
    debug_printf("%s\n", str);

    debug_printf("SEND!\n");
    err = lmp_chan_send1(lc, LMP_SEND_FLAGS_DEFAULT, cap_selfep, LMP_ResponseType_Ok);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

void lmp_server_memory(struct lmp_chan *lc, struct capref cap, size_t align, size_t bytes) {
    
    assert(align > 0);
    assert(bytes > 0);
    
    struct capref ram;
    ram_alloc_aligned(&ram, bytes, align);
    
    // Send ram capability back
    lmp_chan_send1(lc, LMP_SEND_FLAGS_DEFAULT, ram, 42, actual_size);
    
    // TODO: Should we delete the capability and free the slot?
    cap_delete(ram);
    slot_free(ram);
    
}

void lmp_server_spawn(struct lmp_chan *lc, struct capref cap) {

}

void lmp_server_terminal(struct lmp_chan *lc, struct capref cap) {

}

/* ========== Client ========== */


static uint8_t done = 0;
void lmp_client_recv(void *arg) {
    struct lmp_chan *lc = (struct lmp_chan *) arg;

    lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_client_wait, lc));

    debug_printf("BUSY WAITING!\n");
    while (!done) {
//        debug_printf("YIELDING!\n");
//        sys_yield(curdispatcher());
    }
    debug_printf("DONE BUSY WAITING!\n");

    done = 0;
}

void lmp_client_wait(void *arg) {
    debug_printf("HELLO THERE!\n");
    done = 1;
}
