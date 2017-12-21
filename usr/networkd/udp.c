//
//  udp.c
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "udp.h"

#include <stdbool.h>
#include <string.h>

#include <aos/urpc.h>
#include <aos/waitset_chan.h>
#include <aos/event_queue.h>

#include <collections/list.h>

#include <netutil/htons.h>
#include <netutil/checksum.h>

// (SLIP) Should dump every received packet to stdout
extern bool dump_packets;


struct event_queue_pair {
    struct event_queue queue;
    struct event_queue_node node;
};


static void udp_handle_urpc(struct udp_socket *socket, void *buf, size_t size,
                            urpc_msg_type_t msg_type);


// IP address of this host
extern uint32_t host_ip;

// List of registered sockets
static collections_listnode *socket_list;

// Next port to use for port allocation
static uint32_t port_counter = 1024;

// Simple == predicate
static int32_t predicate_equals(void *data, void *arg) {
    return data == arg;
}

// Predicate function for finding an open socket with a specific port
static int32_t predicate_open_socket_with_port(struct udp_socket *socket,
                                               uint16_t *port) {
    return socket->state == UDP_SOCKET_STATE_OPEN && socket->pub.port == *port;
}

// Handler for waitset events
static void dispatch_event_handler(void *arg) {
    
    errval_t err;
    
    // Accept new bind requests
    static struct udp_socket *socket = NULL;
    if (!socket) { socket = malloc(sizeof(struct udp_socket)); assert(socket); }
    err = urpc_accept(&socket->chan);
    if (err_is_ok(err)) {
        // Register a new socket
        socket->state = UDP_SOCKET_STATE_CLOSED;
        collections_list_insert(socket_list, socket);
        socket = malloc(sizeof(struct udp_socket));
        assert(socket);
    }
    else if (err != LIB_ERR_NO_URPC_BIND_REQ) {
        debug_printf("Error in urpc_accept(): %s\n", err_getstring(err));
    }
    
    // Iterate all registered sockets
    struct udp_socket *node;
    collections_list_traverse_start(socket_list);
    while ((node = (struct udp_socket *) collections_list_traverse_next(socket_list)) != NULL) {
        
        // Receive on the socket's channel if possible
        void *buf;
        size_t size;
        urpc_msg_type_t msg_type;
        err = urpc_recv(&node->chan, &buf, &size, &msg_type);
        if (err_is_ok(err)) {
            // Handle message
            udp_handle_urpc(node, buf, size, msg_type);
            free(buf);
        }
        else if (err != LIB_ERR_NO_URPC_MSG) {
            debug_printf("Error in urpc_recv(): %s\n", err_getstring(err));
        }
        
    }
    collections_list_traverse_end(socket_list);
    
    // Reregister event node
    struct event_queue_pair *pair = arg;
    event_queue_add(&pair->queue,
                    &pair->node,
                    MKCLOSURE(dispatch_event_handler, (void *) pair));
    
}

static struct event_queue_pair event_pair;

// Register handler for waitset events
//  When waitset is NULL, only reregisters
void udp_register_event_queue(struct waitset *waitset) {
    
    if (waitset) {
        event_queue_init(&event_pair.queue, waitset, EVENT_QUEUE_CONTINUOUS);
    }
    event_queue_add(&event_pair.queue,
                    &event_pair.node,
                    MKCLOSURE(dispatch_event_handler, (void *) &event_pair));
    
}

// Cancel waitset events
void udp_cancel_event_queue(void) {
    
    event_queue_cancel(&event_pair.queue, &event_pair.node);
    
}

// Initialize the UDP module
void udp_init(void) {
    
    collections_list_create(&socket_list, free);
    
}

// Parse and validate a UDP header
int udp_parse_header(struct ip_packet_header *ip, uint8_t *buf, size_t len,
                     struct udp_header *header) {
    
    uint16_t *buf16 = (uint16_t *) buf;
    
    // Parse all fields
    header->src_port = lwip_ntohs(buf16[0]);
    header->dest_port = lwip_ntohs(buf16[1]);
    header->length = lwip_ntohs(buf16[2]);
    header->checksum = lwip_ntohs(buf16[3]);

    // Compute and verify checksum
    struct udp_checksum_ip_pseudo_header ph;
    ph.src_addr = lwip_htonl(ip->src);
    ph.dest_addr = lwip_htonl(ip->dest);
    ph.zeros = 0;
    ph.protocol = IP_PROTOCOL_UDP;
    ph.udp_length = buf16[2];
    ph.checksum = ~inet_checksum((void *) buf, len);
    
    uint16_t checksum = inet_checksum((void *) &ph, sizeof(struct udp_checksum_ip_pseudo_header));
    if (checksum != 0xFFFF && checksum != 0x0) {
        return 1;
    }
    
    return 0;
    
}

// Encode a UDP header
static int udp_encode_header(struct udp_header *header,
                             struct udp_urpc_packet *packet, uint8_t *out) {
    
    uint16_t *out16 = (uint16_t *) out;
    
    // Set fields
    out16[0] = lwip_htons(header->src_port);
    out16[1] = lwip_htons(header->dest_port);
    out16[2] = lwip_htons(header->length);
    out16[3] = ~header->checksum;
    
    // Compute checksum
    struct udp_checksum_ip_pseudo_header ph;
    ph.src_addr = lwip_htonl(host_ip);
    ph.dest_addr = lwip_htonl(packet->addr);
    ph.zeros = 0;
    ph.protocol = IP_PROTOCOL_UDP;
    ph.udp_length = out16[2];
    ph.checksum = ~inet_checksum((void *) out, 8);
    out16[3] = inet_checksum((void *) &ph, sizeof(struct udp_checksum_ip_pseudo_header));
    
    return 0;
    
}

// Handle an incoming UDP packet
void udp_handle_packet(struct ip_packet_header *ip, uint8_t *buf, size_t len) {
    
    // Discard too small messages
    if (len < 8) {
        debug_printf("INVALID UDP PACKET: TOO SHORT\n");
        return;
    }
    
    // Parse and check the UDP header
    struct udp_header header;
    int e;
    if ((e = udp_parse_header(ip, buf, len, &header))) {
        debug_printf("INVALID UDP PACKET: %d\n", e);
        return;
    }
    
    // Find socket bound to the destination port
    struct udp_socket *socket = collections_list_find_if(socket_list,
                                                         (collections_list_predicate) predicate_open_socket_with_port,
                                                         (void *) &header.dest_port);
    
    // Check that a socket was found
    if (!socket) {
        debug_printf("UDP: Dropping packet\n");
        return; // Drop packet
    }
    
    // Calculate size of message to be forwarded to client process
    size_t msg_size = sizeof(struct udp_urpc_packet) + len - 8;
    
    // Allocate memory to construct the message
    struct udp_urpc_packet *packet = malloc(msg_size);
    if (!packet) {
        debug_printf("UDP: Dropping packet\n");
        return; // Drop packet
    }
    
    // Set source information
    packet->addr = ip->src;
    packet->port = header.src_port;
    packet->size = len - 8;

    // Copy payload
    memcpy(packet->payload, buf + 8, len - 8);
    
    // Forward packet to the process holding the socket
    errval_t err = urpc_send(&socket->chan,
                             (void *) packet,
                             msg_size,
                             URPC_MessageType_Receive);
    if (err_is_fail(err)) {
        debug_printf("Error in urpc_send(): %s\n", err_getstring(err));
    }
    
}

// Open a socket on a given port
static int udp_socket_open(struct udp_socket *socket,
                           struct udp_socket_common *msg) {
    
    // Find a port
    while (true) {
        
        // If no port specified, select a random port
        uint32_t port = msg->port ? msg->port : port_counter++;
        
        // Check if the port is taken
        if (collections_list_find_if(socket_list,
                                     (collections_list_predicate) predicate_open_socket_with_port,
                                     (void *) &port)) {
            // If a specific port was requested, fail
            if (msg->port) {
                return 1;
            }
            else {
                continue;
            }
        }
        
        // Use the port
        msg->port = port;
        
        break;
        
    }
    // Copy over the shared socket data
    memcpy(&socket->pub, msg, sizeof(struct udp_socket_common));
    
    // Open the port
    socket->state = UDP_SOCKET_STATE_OPEN;
    
    return 0;
    
}

// Send a UDP packet
static void udp_socket_send(struct udp_socket *socket,
                           struct udp_urpc_packet *packet, size_t len) {
    
    struct udp_header reply_header;
    
    // Build header
    reply_header.src_port = socket->pub.port;
    reply_header.dest_port = packet->port;
    reply_header.length = packet->size + 8;
    
    // Compute checksum for payload
    reply_header.checksum = inet_checksum((void *) packet->payload,
                                          packet->size);
    
    // Send IP header
    ip_send_header(packet->addr, IP_PROTOCOL_UDP, reply_header.length);
    
    // Encode header and send it
    uint8_t *reply_buf = malloc(8);
    assert(reply_buf);
    udp_encode_header(&reply_header, packet, reply_buf);
    ip_send(reply_buf, 8, false);
    free(reply_buf);
    
    // Send payload
    ip_send((uint8_t *) packet->payload, packet->size, true);
    
}

// Handle a URPC message from a client process
static void udp_handle_urpc(struct udp_socket *socket, void *buf, size_t size,
                            urpc_msg_type_t msg_type) {
    
    errval_t err;
    
    switch (msg_type) {
        case URPC_MessageType_SocketOpen:
            assert(sizeof(struct udp_socket_common) <= size);
            int e;
            if (!(e = udp_socket_open(socket,
                                      (struct udp_socket_common *) buf))) {
                err = urpc_send(&socket->chan,
                                (void *) &socket->pub,
                                sizeof(struct udp_socket_common),
                                URPC_MessageType_SocketOpen);
                if (err_is_fail(err)) {
                    debug_printf("Error in urpc_send(): %s\n",
                                 err_getstring(err));
                }
            }
            else {
                err = urpc_send(&socket->chan,
                                (void *) &e,
                                sizeof(int),
                                URPC_MessageType_Error);
                if (err_is_fail(err)) {
                    debug_printf("Error in urpc_send(): %s\n",
                                 err_getstring(err));
                }
            }
            break;
            
        case URPC_MessageType_Send:
            udp_socket_send(socket,
                            (struct udp_urpc_packet *) buf,
                            size);
            break;
            
        case URPC_MessageType_SocketClose:
            collections_list_traverse_end(socket_list);
            collections_list_remove_if(socket_list, predicate_equals, socket);
            collections_list_traverse_start(socket_list);
            break;
            
        // For network utilites
        case URPC_MessageType_SetIPAddress:
            ip_set_ip_address(((uint8_t *) buf)[0],
                              ((uint8_t *) buf)[1],
                              ((uint8_t *) buf)[2],
                              ((uint8_t *) buf)[3]);
            break;
        case URPC_MessageType_DumpPackets:
            dump_packets = *(bool *) buf;
            break;
            
        default:
            debug_printf("Received unknown URPC message type: %d\n", msg_type);
            break;
    }
    
}
