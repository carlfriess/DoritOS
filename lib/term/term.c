//
//  terminal.c
//  DoritOS
//

#include <stdlib.h>

#include <aos/aos.h>

#include <term/term.h>


#define PRINT_DEBUG 0


void io_buffer_init(struct io_buffer **buf) {
    *buf = (struct io_buffer *) malloc(sizeof(struct io_buffer));
    (*buf)->buf = (char *) malloc(IO_BUFFER_SIZE);
    (*buf)->pos = 0;
    (*buf)->next = NULL;
    memset((*buf)->buf, 0, IO_BUFFER_SIZE);
}

static errval_t get_next_char(struct terminal_state *st, char *c) {
    static size_t i = 0;
    
    if (i >= st->buffer->pos) {
        return TERM_ERR_BUFFER_EMPTY;
    }
    
    *c = st->buffer->buf[i++];
    
    if (i == IO_BUFFER_SIZE) {
        struct io_buffer *temp = st->buffer->next;
        free(st->buffer);
        st->buffer = temp;
        i = 0;
    }
    
    return SYS_ERR_OK;
}

static int urpc_chan_list_remove(void *data, void *arg) {
    return data == arg;
}

static int terminal_event_dispatch(void *data, void *arg) {
        
    errval_t err = SYS_ERR_OK;
    
    struct terminal_state *st = (struct terminal_state *) arg;
    
    struct terminal_event *event = (struct terminal_event *) data;
    
    struct terminal_msg msg = {
        .err = SYS_ERR_OK
    };
    
    switch (event->type) {
        case URPC_MessageType_TerminalReadLock:
#if PRINT_DEBUG
            debug_printf("EVENT: READ LOCK!\n");
#endif
            if (st->read_lock == NULL || st->read_lock == event->chan) {
                st->read_lock = event->chan;
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
            if (st->read_lock == NULL || st->read_lock == event->chan) {
                msg.err = get_next_char(st, &msg.c);
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
            if (st->read_lock == NULL || st->read_lock == event->chan) {
                st->read_lock = NULL;
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
            if (st->write_lock == NULL || st->write_lock == event->chan) {
                st->write_lock = event->chan;
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
            if (st->write_lock == NULL || st->write_lock == event->chan) {
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
            if (st->write_lock == NULL || st->write_lock == event->chan) {
                st->write_lock = NULL;
            }
            err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
            return 1;
        case URPC_MessageType_TerminalDeregister:
#if PRINT_DEBUG
            debug_printf("EVENT: Deregister!\n");
#endif
            collections_list_remove_if(st->urpc_chan_list, urpc_chan_list_remove, event->chan);
            err = urpc_send(event->chan, (void *) &msg, sizeof(struct terminal_msg), event->type);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
#if PRINT_DEBUG
            debug_printf("Registered Processes: %d", collections_list_size(st->urpc_chan_list));
#endif
            
            return 1;
            break;
    }
    
    return 0;
}

void terminal_runloop(struct terminal_state *st) {
    
    errval_t err;
    
    io_buffer_init(&st->buffer);
    
    collections_list_create(&st->urpc_chan_list, free);
    
    collections_list_create(&st->terminal_event_queue, free);
    
    terminal_ready();
    
    void *chan = malloc(sizeof(struct urpc_chan));
    while (true) {
        while (collections_list_remove_if(st->terminal_event_queue, terminal_event_dispatch, st) != NULL);
        
        err = urpc_accept((struct urpc_chan *) chan);
        if (err_is_ok(err)) {
            collections_list_insert(st->urpc_chan_list, chan);
            chan = malloc(sizeof(struct urpc_chan));
        }
        else if (err != LIB_ERR_NO_URPC_BIND_REQ) {
            debug_printf("Error in urpc_accept(): %s\n", err_getstring(err));
        }

        void *node;
        struct terminal_msg *msg;
        size_t size;
        urpc_msg_type_t msg_type;
        
        collections_list_traverse_start(st->urpc_chan_list);
        while ((node = collections_list_traverse_next(st->urpc_chan_list)) != NULL) {
            err = urpc_recv(node, (void *) &msg, &size, &msg_type);
            
            if (err_is_ok(err)) {
                struct terminal_event *event = (struct terminal_event *) malloc(sizeof(struct terminal_event));
                event->chan = node;
                event->type = msg_type;
                event->msg = *msg;
                collections_list_insert_tail(st->terminal_event_queue, event);
                
                free((void *) msg);
            }
            else if (err != LIB_ERR_NO_URPC_MSG) {
                debug_printf("Error in urpc_recv(): %s\n", err_getstring(err));
            }
        }
        collections_list_traverse_end(st->urpc_chan_list);
        
        terminal_runloop_dispatch();
        
    }
    
}
