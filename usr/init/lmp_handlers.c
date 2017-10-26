#include <aos/aos.h>
#include <aos/waitset.h>

#include "lmp_handlers.h"

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

enum lmp_messagetype {
    LMP_MessageType_NULL = 0,
    LMP_MessageType_Register,
    LMP_MessageType_Memory,
    LMP_MessageType_Spawn,
    LMP_MessageType_Terminal
};

void lmp_handler_dispatcher(void *arg) {

    debug_printf("LMP Message Received!\n");

    errval_t err;

    struct lmp_chan *lc = (struct lmp_chan *) arg;
    struct capref *cap = NULL;
    struct lmp_recv_msg *msg = NULL;

    err = lmp_chan_recv(lc, msg, cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    // Check message type and handle
    switch (msg->words[0]) {
        case LMP_MessageType_Register:
            lmp_handler_register(lc, cap);
            break;
        case LMP_MessageType_Memory:

            break;
        case LMP_MessageType_Spawn:

            break;
        case LMP_MessageType_Terminal:

            break;
    }

    // TODO: REREGISTER
}

void lmp_handler_register(struct lmp_chan *lc, struct capref *cap) {

}

void lmp_handler_memory(struct lmp_chan *lc, struct capref *cap, size_t align, size_t bytes) {

}

void lmp_handler_spawn(struct lmp_chan *lc, struct capref *cap) {

}

void lmp_handler_terminal(struct lmp_chan *lc, struct capref *cap) {

}