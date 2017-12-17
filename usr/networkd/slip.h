//
//  slip.h
//  DoritOS
//
//  Created by Carl Friess on 16/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef slip_h
#define slip_h

#include <stdio.h>
#include <stdint.h>

#define SLIP_END        0xc0
#define SLIP_ESC        0xdb
#define SLIP_ESC_END    0xdc
#define SLIP_ESC_ESC    0xdd
#define SLIP_ESC_NUL    0xde

#define MAX_IP_PACKET_SIZE  65535

struct ip_packet_raw {
    uint8_t *buf;
    size_t len;
};

int slip_init(void);
void slip_recv(uint8_t *buf, size_t len);

#endif /* slip_h */
