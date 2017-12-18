//
//  udp_socket.h
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef udp_socket_h
#define udp_socket_h

#include <net/common.h>

#include <stdint.h>


struct udp_socket {
    struct udp_socket_common pub;
    struct urpc_chan chan;
};


// Open a UDP socket and optionally bind to a specific port
//  Specify port 0 to bind on random port.
errval_t socket(struct udp_socket *socket, uint16_t port);

// Blockingly receive a UDP packet on the given socket
errval_t recvfrom(struct udp_socket *socket, void *buf, size_t len,
                  size_t *ret_len, uint32_t *from_addr, uint16_t *from_port);

// Send a UDP packet on the given socket to a specific destination
errval_t sendto(struct udp_socket *socket, void *buf, size_t len,
                uint32_t to_addr, uint16_t to_port);

#endif /* udp_socket_h */
