#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/lmp.h>

#define PRINT_DEBUG 1

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
#if PRINT_DEBUG
            debug_printf("Short String Message!\n");
#endif
            debug_printf("Received string: %s\n", (char *)(msg.words+1));
            lmp_chan_send3(lc,
                           LMP_SEND_FLAGS_DEFAULT,
                           NULL_CAP,
                           LMP_RequestType_StringShort,
                           SYS_ERR_OK,
                           strlen((char *)(msg.words+1)));
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
        case LMP_RequestType_Terminal:
#if PRINT_DEBUG
            debug_printf("Terminal Message!\n");
#endif
            break;
        default:
#if PRINT_DEBUG
            debug_printf("Invalid Message!\n");
#endif
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

void lmp_server_memory_alloc(struct lmp_chan *lc, size_t bytes, size_t align) {

}
void lmp_server_memory_free(struct lmp_chan *lc, struct capref cap) {

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

void lmp_server_terminal(struct lmp_chan *lc, struct capref cap) {

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

    lmp_chan_recv(lc, msg, cap);
}

void lmp_client_wait(void *arg) {
    *(int *)arg = 1;
}
