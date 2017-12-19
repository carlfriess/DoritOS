//
//  udp_socket.c
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <net/common.h>
#include <net/udp_socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aos/aos.h>
#include <aos/aos_rpc.h>


// Bind to networkd
static errval_t bind_networkd(struct urpc_chan *chan) {
    
    errval_t err;
    
    // Find networkd's pid
    domainid_t pid = 0;
    err = aos_rpc_process_get_pid_by_name("networkd", &pid);
    if (err_is_fail(err)) {
        free(chan);
        chan = NULL;
        return err;
    }
    
    // Try to bind to networkd
    //  Use LMP when on core 0!
    err = urpc_bind(pid, chan, !disp_get_core_id());
    if (err_is_fail(err)) {
        free(chan);
        chan = NULL;
        return err;
    }
    
    return SYS_ERR_OK;
    
}

// Open a UDP socket and optionally bind to a specific port
//  Specify port 0 to bind on random port.
errval_t socket(struct udp_socket *socket, uint16_t port) {
    
    errval_t err;
    
    // Bind to networkd
    err = bind_networkd(&socket->chan);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Set port the socket should bind to
    socket->pub.port = port;
    
    // Send URPC messages
    err = urpc_send(&socket->chan,
                    (void *) &socket->pub,
                    sizeof(struct udp_socket_common),
                    URPC_MessageType_SocketOpen);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Wait for response
    void *buf;
    size_t size;
    urpc_msg_type_t msg_type;
    err = urpc_recv_blocking(&socket->chan, &buf, &size, &msg_type);
    if (err_is_fail(err)) {
        return err;
    }
    
    if (msg_type == URPC_MessageType_SocketOpen) {
        assert(size == sizeof(struct udp_socket_common));
        memcpy(&socket->pub, buf, size);
        free(buf);
        return SYS_ERR_OK;
    }
    else if (msg_type == URPC_MessageType_Error) {
        return NET_ERR_SOCKET_OPEN;
    }
    else {
        return NET_ERR_INVALID_URPC;
    }
    
}

// Blockingly receive a UDP packet on the given socket
errval_t recvfrom(struct udp_socket *socket, void *buf, size_t len,
                  size_t *ret_len, uint32_t *from_addr, uint16_t *from_port) {
    
    // Receive a packet over URPC
    struct udp_urpc_packet *packet;
    size_t size;
    urpc_msg_type_t msg_type;
    errval_t err = urpc_recv_blocking(&socket->chan, (void **) &packet, &size,
                                      &msg_type);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Check message type
    if (msg_type == URPC_MessageType_Receive) {
        
        // Extract data
        *from_addr = packet->addr;
        *from_port = packet->port;
        *ret_len = size - sizeof(struct udp_urpc_packet);
        memcpy(buf, packet->payload, MIN(len, *ret_len));
        free(packet);
        return SYS_ERR_OK;
        
    }
    else {
        
        return NET_ERR_INVALID_URPC;
        
    }
    
}

// Send a UDP packet on the given socket to a specific destination
errval_t sendto(struct udp_socket *socket, void *buf, size_t len,
                uint32_t to_addr, uint16_t to_port) {
    
    errval_t err;
    
    // Calculate message size
    size_t msg_size = sizeof(struct udp_urpc_packet) + len;
    
    // Allocate memory for the message
    struct udp_urpc_packet *packet = malloc(msg_size);
    if (!packet) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Set destination
    packet->addr = to_addr;
    packet->port = to_port;
    
    // Copy message
    memcpy(packet->payload, buf, len);
    
    // Send the message to networkd
    err = urpc_send(&socket->chan, packet, msg_size, URPC_MessageType_Send);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Free message
    free(packet);
    
    return SYS_ERR_OK;
    
}

// Close a UDP socket
errval_t close(struct udp_socket *socket) {
    
    // Send message to networkd
    return urpc_send(&socket->chan, NULL, 0, URPC_MessageType_SocketClose);
    
}

