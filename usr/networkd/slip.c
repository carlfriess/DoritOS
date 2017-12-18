//
//  slip.c
//  DoritOS
//
//  Created by Carl Friess on 16/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "slip.h"
#include "ip.h"
#include "udp.h"

#include <stdlib.h>
#include <string.h>
#include <bitmacros.h>
#include <assert.h>

#include <aos/aos.h>

#include <netutil/user_serial.h>


static void slip_parse_raw_ip_packet(struct ip_packet_raw *raw_packet);


static struct ip_packet_raw current_packet = {
    .buf = NULL,
    .len = 0
};

static enum {
    PARSER_STATE_IDLE,
    PARSER_STATE_NORMAL,
    PARSER_STATE_ESC
} parser_state = PARSER_STATE_IDLE;

int slip_init(void) {
    
    // Allocating space for receive buffer (max ip packet size)
    current_packet.buf = calloc(MAX_IP_PACKET_SIZE, 1);
    current_packet.len = 0;
    if (!current_packet.buf) {
        return -1;
    }
    
    // Sending SLIP_END to initialize communication
    uint8_t esc_buf[] = { SLIP_END };
    serial_write(esc_buf, 1);
    
    return 0;
    
}

// Receive and parse bytes from the network
void slip_recv(uint8_t *buf, size_t len) {
    
    // Bring the parser out of idle
    if (parser_state == PARSER_STATE_IDLE) {
        parser_state = PARSER_STATE_NORMAL;
        // Cancel UDP events
        udp_cancel_event_queue();
    }
    
    // Iterate through all received bytes
    while (len--) {
        //debug_printf("%x, %zu\n", *buf, current_packet.len);
        switch (*buf) {
            
            // Handle end of packet
            case SLIP_END:
                // Sanity check
                assert(parser_state == PARSER_STATE_NORMAL);
                
                // Parse the raw IP packet
                slip_parse_raw_ip_packet(&current_packet);
                
                // Reset the input parser
                parser_state = PARSER_STATE_IDLE;
                current_packet.buf -= current_packet.len;
                current_packet.len = 0;
                
                // Allow UDP events again
                udp_register_event_queue(NULL);
                
                continue;
            
            // Handle escape sequences
            case SLIP_ESC:
                assert(parser_state == PARSER_STATE_NORMAL);
                parser_state = PARSER_STATE_ESC;
                buf++;
                continue;
                
            case SLIP_ESC_END:
                if (parser_state == PARSER_STATE_ESC) {
                    *buf = SLIP_END;
                    parser_state = PARSER_STATE_NORMAL;
                }
                break;
                
            case SLIP_ESC_ESC:
                if (parser_state == PARSER_STATE_ESC) {
                    *buf = SLIP_ESC;
                    parser_state = PARSER_STATE_NORMAL;
                }
                break;
                
            case SLIP_ESC_NUL:
                if (parser_state == PARSER_STATE_ESC) {
                    *buf = 0x00;
                    parser_state = PARSER_STATE_NORMAL;
                }
                break;
            
            default:
                // Make sure escape sequences are correct
                assert(parser_state == PARSER_STATE_NORMAL);
        }
        
        // Copy the received byte
        *(current_packet.buf++) = *(buf++);
        current_packet.len++;

    }
    
}

// Send buffer over network
void slip_send(uint8_t *buf, size_t len, bool end) {

    uint8_t esc_buf[2] = {SLIP_ESC, 0x00};

    for (int i = 0; i < len; i++) {
        
        switch (buf[i]) {
                
            case SLIP_END:
                esc_buf[1] = SLIP_ESC_END;
                serial_write(esc_buf, 2);
                break;
                
            case SLIP_ESC:
                esc_buf[1] = SLIP_ESC_ESC;
                serial_write(esc_buf, 2);
                break;
                
            case 0x00:
                esc_buf[1] = SLIP_ESC_NUL;
                serial_write(esc_buf, 2);
                break;
                
            default:
                serial_write(buf + i, 1);
                break;
            
        }
        
    }
    
    if (end) {
        esc_buf[0] = SLIP_END;
        serial_write(esc_buf, 1);
    }
    
}


static void slip_parse_raw_ip_packet(struct ip_packet_raw *raw_packet) {
    
    // Discard empty packets
    if (raw_packet->len == 0) {
        return;
    }
    
    ip_handle_packet(raw_packet->buf - raw_packet->len, raw_packet->len);
    
};
