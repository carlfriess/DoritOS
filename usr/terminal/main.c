#include <stdio.h>
#include <stdlib.h>

#include <collections/list.h>
#include <driverkit/driverkit.h>
#include <maps/omap44xx_map.h>

#include <aos/aos_rpc.h>
#include <aos/inthandler.h>
#include <aos/urpc.h>

struct io_buffer {
    char *buf;
    size_t pos;
    struct io_buffer *next;
};

// Memory location to read/write from
volatile char *mem;

// Flag byte to check for read/write
volatile char *flag;

const size_t IO_BUFFER_SIZE = 5;

struct io_buffer *io_buffer = NULL;

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

static errval_t get_next_char(char *c) {
    static size_t i = 0;

    if (i == io_buffer->pos) {
        return TERM_ERR_BUFFER_EMPTY;
    }

    *c = io_buffer->buf[i++];

    if (i == IO_BUFFER_SIZE) {
        struct io_buffer *temp = io_buffer->next;
        free(io_buffer);
        io_buffer = temp;
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
    char c = get_char();

    // Iterate to tail
    struct io_buffer *current;
    for (current = io_buffer; current->next != NULL; current = current->next);

    // Set char and inc
    current->buf[current->pos++] = c;

    // Alloc new list if full
    if (current->pos == IO_BUFFER_SIZE) {
        io_buffer_init(&current->next);
    }
}

int main(int argc, char *argv[]) {
    errval_t err;

    io_buffer_init(&io_buffer);

    // Register Serial Interrupt Handler
    err = inthandler_setup_arm(serial_interrupt_handler, NULL, 106);
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

    terminal_ready();

    void *session_id_write = NULL;
    void *session_id_read = NULL;

    void *chan = malloc(sizeof(struct urpc_chan));
    while (true) {
        err = urpc_accept((struct urpc_chan *) chan);
        if (err != LIB_ERR_NO_URPC_BIND_REQ) {
            collections_list_insert(urpc_chan_list, chan);
            chan = malloc(sizeof(struct urpc_chan));
        }

        void *node;
        struct terminal_msg *in;
        struct terminal_msg out;
        size_t size;
        urpc_msg_type_t msg_type;

        collections_list_traverse_start(urpc_chan_list);
        while ((node = collections_list_traverse_next(urpc_chan_list)) != NULL) {
            err = urpc_recv(node, (void *) &in, &size, &msg_type);

            if (err == SYS_ERR_OK) {
                switch (msg_type) {
                    case URPC_MessageType_TerminalWrite:
                        if (session_id_write == NULL) {
                            out.err = SYS_ERR_OK;
                            if (in->lock) {
                                session_id_write = node;
                            } else {
                                put_char(in->c);
                            }
                        } else if (session_id_write == node) {
                            out.err = SYS_ERR_OK;
                            if (!in->lock) {
                                put_char(in->c);
                            }
                        } else {
                            out.err = TERM_ERR_TERMINAL_IN_USE;
                        }

                        urpc_send(node, (void *) &out, sizeof(struct terminal_msg), URPC_MessageType_TerminalWrite);
                        break;
                    case URPC_MessageType_TerminalWriteUnlock:
                        if (session_id_write == NULL) {
                            out.err = SYS_ERR_OK;
                        } else if (session_id_write == node) {
                            session_id_write = NULL;
                            out.err = SYS_ERR_OK;
                        } else {
                            out.err = TERM_ERR_TERMINAL_IN_USE;
                        }

                        urpc_send(node, (void *) &out, sizeof(struct terminal_msg), URPC_MessageType_TerminalWriteUnlock);
                        break;
                    case URPC_MessageType_TerminalRead:
                        if (session_id_read == NULL) {
                            if (!in->lock) {
                                out.err = get_next_char(&out.c);
                            } else {
                                session_id_read = node;
                                out.err = SYS_ERR_OK;
                            }
                        } else if (session_id_read == node) {
                            if (!in->lock) {
                                out.err = get_next_char(&out.c);
                            } else {
                                out.err = SYS_ERR_OK;
                            }
                        } else {
                            out.err = TERM_ERR_TERMINAL_IN_USE;
                        }

                        urpc_send(node, (void *) &out, sizeof(struct terminal_msg), URPC_MessageType_TerminalRead);
                        break;
                    case URPC_MessageType_TerminalReadUnlock:
                        if (session_id_read == NULL) {
                            out.err = SYS_ERR_OK;
                        } else if (session_id_read == node) {
                            session_id_read = NULL;
                            out.err = SYS_ERR_OK;
                        } else {
                            out.err = TERM_ERR_TERMINAL_IN_USE;
                        }

                        urpc_send(node, (void *) &out, sizeof(struct terminal_msg), URPC_MessageType_TerminalReadUnlock);
                        break;
                }
                free((void *) in);
            }
        }
        collections_list_traverse_end(urpc_chan_list);

        event_dispatch_non_block(get_default_waitset());
    }
}
