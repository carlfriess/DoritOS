#include <stdio.h>
#include <stdlib.h>

#include <collections/list.h>
#include <driverkit/driverkit.h>
#include <maps/omap44xx_map.h>

#include <aos/terminal.h>
#include <aos/aos_rpc.h>
#include <aos/inthandler.h>
#include <aos/urpc.h>

#define PRINT_DEBUG 0

#define IO_BUFFER_SIZE  256


struct io_buffer {
    char *buf;
    size_t pos;
    struct io_buffer *next;
};

struct terminal_state {
    void *write_lock;
    void *read_lock;
    struct io_buffer *buffer;
};

// Memory location to read/write from
volatile char *mem;

// Flag byte to check for read/write
volatile char *flag;

static void io_buffer_init(struct io_buffer **buf) {
    *buf = (struct io_buffer *) malloc(sizeof(struct io_buffer));
    (*buf)->buf = (char *) malloc(IO_BUFFER_SIZE);
    (*buf)->pos = 0;
    (*buf)->next = NULL;
    memset((*buf)->buf, 0, IO_BUFFER_SIZE);
}

static void put_char(char c) {
    while (!(*flag & 0x20));
    *mem = c;
}

static char get_char(void) {
    while (!(*flag & 0x1));
    return *mem;
}

static errval_t get_next_char(struct terminal_state *state, char *c) {
    static size_t i = 0;

    if (i >= state->buffer->pos) {
        return TERM_ERR_BUFFER_EMPTY;
    }

    *c = state->buffer->buf[i++];

    if (i == IO_BUFFER_SIZE) {
        struct io_buffer *temp = state->buffer->next;
        free(state->buffer);
        state->buffer = temp;
        i = 0;
    }

    return SYS_ERR_OK;
}

static void terminal_ready(void) {
    domainid_t pid;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "networkd", 0, &pid);
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "shell", 0, &pid);
}

static void serial_interrupt_handler(void *arg) {
    struct terminal_state *state = (struct terminal_state *) arg;

    char c = get_char();

    // Iterate to tail
    struct io_buffer *current;
    for (current = state->buffer; current->next != NULL; current = current->next);

    // Set char and inc
    current->buf[current->pos++] = c;

    // Alloc new list if full
    if (current->pos == IO_BUFFER_SIZE) {
        io_buffer_init(&current->next);
    }
}


struct terminal_event {
    urpc_msg_type_t type;
    struct urpc_chan *chan;
    struct terminal_msg msg;
};

static int terminal_event_dispatch(void *data, void *arg) {
    errval_t err = SYS_ERR_OK;

    struct terminal_state *state = (struct terminal_state *) arg;

    struct terminal_event *event = (struct terminal_event *) data;

    struct terminal_msg msg = {
        .err = SYS_ERR_OK
    };

    switch (event->type) {
        case URPC_MessageType_TerminalReadLock:
#if PRINT_DEBUG
            debug_printf("EVENT: READ LOCK!\n");
#endif
            if (state->read_lock == NULL || state->read_lock == event->chan) {
                state->read_lock = event->chan;
                err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                return 1;
            }
            break;
        case URPC_MessageType_TerminalRead:
#if PRINT_DEBUG
            debug_printf("EVENT: READ!\n");
#endif
            if (state->read_lock == NULL || state->read_lock == event->chan) {
                msg.err = get_next_char(state, &msg.c);
                if (msg.err == SYS_ERR_OK) {
                    err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
                    if (err_is_fail(err)) {
                        debug_printf("%s\n", err_getstring(err));
                    }
                    return 1;
                }
            }
            break;
        case URPC_MessageType_TerminalReadUnlock:
#if PRINT_DEBUG
            debug_printf("EVENT: READ UNLOCK!\n");
#endif
            if (state->read_lock == NULL || state->read_lock == event->chan) {
                state->read_lock = NULL;
            }
            err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            return 1;
        case URPC_MessageType_TerminalWriteLock:
#if PRINT_DEBUG
            debug_printf("EVENT: WRITE LOCK!\n");
#endif
            if (state->write_lock == NULL || state->write_lock == event->chan) {
                state->write_lock = event->chan;
                err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                return 1;
            }
            break;
        case URPC_MessageType_TerminalWrite:
#if PRINT_DEBUG
            debug_printf("EVENT: WRITE!\n");
#endif
            if (state->write_lock == NULL || state->write_lock == event->chan) {
                put_char(event->msg.c);
                err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
                if (err_is_fail(err)) {
                    debug_printf("%s\n", err_getstring(err));
                }
                return 1;
            }
            break;
        case URPC_MessageType_TerminalWriteUnlock:
#if PRINT_DEBUG
            debug_printf("EVENT: WRITE UNLOCK!\n");
#endif
            if (state->write_lock == NULL || state->write_lock == event->chan) {
                state->write_lock = NULL;
            }
            err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    errval_t err;

    struct terminal_state state = {
        .write_lock = NULL,
        .read_lock = NULL,
        .buffer = NULL,
    };

    io_buffer_init(&state.buffer);

    // Register Serial Interrupt Handler
    err = inthandler_setup_arm(serial_interrupt_handler, &state, 106);
    if (err_is_fail(err)) {
        debug_printf(err_getstring(err));
        return 1;
    }

    // Get and map Serial device cap;
    lvaddr_t vaddr;
    err = map_device_register(OMAP44XX_MAP_L4_PER_UART3, OMAP44XX_MAP_L4_PER_UART3_SIZE, &vaddr);
    if (err_is_fail(err)) {
        debug_printf(err_getstring(err));
    }

    mem = (char *) vaddr;
    flag = (char *) vaddr + 0x14;

    collections_listnode *urpc_chan_list;
    collections_list_create(&urpc_chan_list, free);

    collections_listnode *terminal_event_queue;
    collections_list_create(&terminal_event_queue, free);

    terminal_ready();

    void *chan = malloc(sizeof(struct urpc_chan));
    while (true) {
        while (collections_list_remove_if(terminal_event_queue, terminal_event_dispatch, &state) != NULL);

        err = urpc_accept((struct urpc_chan *) chan);
        if (err != LIB_ERR_NO_URPC_BIND_REQ) {
            collections_list_insert(urpc_chan_list, chan);
            chan = malloc(sizeof(struct urpc_chan));
        }

        void *node;
        struct terminal_msg *msg;
        size_t size;
        urpc_msg_type_t msg_type;

        collections_list_traverse_start(urpc_chan_list);
        while ((node = collections_list_traverse_next(urpc_chan_list)) != NULL) {
            err = urpc_recv(node, (void *) &msg, &size, &msg_type);

            if (err_is_ok(err)) {
                struct terminal_event *event = (struct terminal_event *) malloc(sizeof(struct terminal_event));
                event->chan = node;
                event->type = msg_type;
                event->msg = *msg;
                collections_list_insert_tail(terminal_event_queue, event);
                free((void *) msg);
            }
        }
        collections_list_traverse_end(urpc_chan_list);

        event_dispatch_non_block(get_default_waitset());
    }
}
