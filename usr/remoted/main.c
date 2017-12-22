//
//  main.c
//  DoritOS
//
//  Created by Carl Friess on 21/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>

#include <aos/aos.h>
#include <aos/aos_rpc.h>
#include <aos/urpc.h>

#include <net/udp_socket.h>

#include <term/term.h>


// Terminal state
struct terminal_state state = TERMINAL_STATE_INIT;

// UDP socket for remote login
static struct udp_socket s;

// UDP port for remote login
static uint16_t port = 23;

// Host used to send terminal output to
static uint32_t from_addr;
static uint16_t from_port;

// Transmit buffer
#define TX_BUF_LEN_MAX 64
char *tx_buf = NULL;
size_t tx_buf_len = 0;


void terminal_ready(void) {
    
    errval_t err = socket(&s, port);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        exit(1);
    }
    
    printf("Socket open on port %d\n", s.pub.port);
    
    // Receive an initial packet to set destination
    char buf[1];
    size_t ret_size;
    err = recvfrom(&s,
                   (void *) buf,
                   sizeof(buf),
                   &ret_size,
                   &from_addr,
                   &from_port);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return;
    }
    
    // Spawn the shell and show the user the beautiful turtle
    domainid_t pid;
    aos_rpc_process_spawn_with_terminal(aos_rpc_get_init_channel(), "shell", 1, disp_get_domain_id(), &pid);
    
}

void put_char(char c) {
    
    // Add the character to the TX buffer
    tx_buf[tx_buf_len++] = c;
    
    // Ckeck if we reached the end of a line or the buffer is full
    if (c == '\n' ||
        (tx_buf[tx_buf_len - 2] == '$' && tx_buf[tx_buf_len - 1] == ' ') ||
        tx_buf_len == TX_BUF_LEN_MAX) {
        
        // Send the buffer
        errval_t err = sendto(&s, tx_buf, tx_buf_len, from_addr, from_port);
        if (err_is_fail(err)) {
            debug_printf("Error in sendto(): %s\n", err_getstring(err));
            return;
        }
        
        // Reset the buffer
        tx_buf_len = 0;
        
        // Slow stuff down
        //for (volatile int i = 0; i < 1000; i++) ;
        
    }
    
}

void terminal_runloop_dispatch(void) {
    
    // Receive a packet over URPC
    struct udp_urpc_packet *packet;
    size_t size;
    urpc_msg_type_t msg_type;
    errval_t err = urpc_recv(&s.chan, (void **) &packet, &size, &msg_type);
    if (err_is_ok(err)) {
        
        from_addr = packet->addr;
        from_port = packet->port;
        
        // Iterate to tail
        struct io_buffer *current;
        for (current = state.buffer; current->next != NULL; current = current->next);
        
        // Iterate all received characters
        for (size_t i = 0; i < packet->size; i++) {
            
            // Set char and inc
            current->buf[current->pos++] = packet->payload[i];
            
            // Alloc new list if full
            if (current->pos == IO_BUFFER_SIZE) {
                io_buffer_init(&current->next);
                current = current->next;
            }
            
        }
        
        // Free the packet
        free(packet);
        
    }
    
}

int main(int argc, char *argv[]) {
    
    // Optionally get port argument
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Initilize TX buf
    tx_buf = malloc(TX_BUF_LEN_MAX);
    assert(tx_buf);
    
    // Run the terminal main loop
    terminal_runloop(&state);
    
}
