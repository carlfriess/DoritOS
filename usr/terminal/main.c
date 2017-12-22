#include <stdio.h>
#include <stdlib.h>

#include <aos/aos_rpc.h>
#include <aos/inthandler.h>

#include <driverkit/driverkit.h>
#include <maps/omap44xx_map.h>

#include <term/term.h>


// Memory location to read/write from
volatile char *mem;

// Flag byte to check for read/write
volatile char *flag;


void put_char(char c) {
    while (!(*flag & 0x20));
    *mem = c;
}

static char get_char(void) {
    while (!(*flag & 0x1));
    return *mem;
}

static void serial_interrupt_handler(void *arg) {
    struct terminal_state *st = (struct terminal_state *) arg;

    char c = get_char();

    // Iterate to tail
    struct io_buffer *current;
    for (current = st->buffer; current->next != NULL; current = current->next);

    // Set char and inc
    current->buf[current->pos++] = c;

    // Alloc new list if full
    if (current->pos == IO_BUFFER_SIZE) {
        io_buffer_init(&current->next);
    }
}

void terminal_ready(void) {
    domainid_t pid;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "networkd", 0, &pid);
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "shell", 0, &pid);
}

void terminal_runloop_dispatch(void) {
    
    event_dispatch_non_block(get_default_waitset());
    
}


int main(int argc, char *argv[]) {
    errval_t err;
    
    // Initilize the terminal state
    struct terminal_state state = TERMINAL_STATE_INIT;

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

    // Run the terminal main loop
    terminal_runloop(&state);
    
}
