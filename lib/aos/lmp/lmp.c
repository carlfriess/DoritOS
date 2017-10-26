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
            debug_printf("Registration Message!\n");
            lmp_server_register(lc, cap);
            break;
        case LMP_RequestType_Memory:
            debug_printf("Memory Message!\n");

            break;
        case LMP_RequestType_Spawn:
            debug_printf("Spawn Message!\n");

            break;
        case LMP_RequestType_Terminal:
            debug_printf("Terminal Message!\n");

            break;
        default:
            debug_printf("Invalid Message!\n");
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
    err = lmp_chan_send1(lc, LMP_SEND_FLAGS_DEFAULT, NULL_CAP, LMP_ResponseType_Ok);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
}

void lmp_server_memory(struct lmp_chan *lc, struct capref cap, size_t align, size_t bytes) {

}

void lmp_server_spawn(struct lmp_chan *lc, struct capref cap) {

}

void lmp_server_terminal(struct lmp_chan *lc, struct capref cap) {

}

/* ========== Client ========== */

static uint8_t done = 0;
void lmp_client_recv(struct lmp_chan *arg, struct capref *cap, struct lmp_recv_msg *msg) {
    struct lmp_chan *lc = (struct lmp_chan *) arg;

    lmp_chan_register_recv(lc, get_default_waitset(), MKCLOSURE(lmp_client_wait, lc));

    while (!done) {
        event_dispatch(get_default_waitset());
    }

    done = 0;
    lmp_chan_recv(lc, msg, cap);
}

void lmp_client_wait(void *arg) {
    done = 1;
}