//
//  terminal.h
//  DoritOS
//


#ifndef lib_terminal_h
#define lib_terminal_h

#include <aos/urpc.h>
#include <aos/terminal.h>

#include <collections/list.h>


#define IO_BUFFER_SIZE  256

#define TERMINAL_STATE_INIT {\
                                .write_lock = NULL,\
                                .read_lock = NULL,\
                                .buffer = NULL,\
                            };


struct io_buffer {
    char *buf;
    size_t pos;
    struct io_buffer *next;
};

struct terminal_state {
    void *write_lock;
    void *read_lock;
    struct io_buffer *buffer;
    collections_listnode *urpc_chan_list;
    collections_listnode *terminal_event_queue;
};

struct terminal_event {
    urpc_msg_type_t type;
    struct urpc_chan *chan;
    struct terminal_msg msg;
};


void io_buffer_init(struct io_buffer **buf);
void terminal_runloop(struct terminal_state *st);

// Need to be implemented:
void terminal_ready(void);
void put_char(char c);
char get_char(void);
void terminal_runloop_dispatch(void);

#endif /* lib_terminal_h */
