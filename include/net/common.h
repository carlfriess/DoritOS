//
//  common.h
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef common_h
#define common_h

#include <stdint.h>

#include <aos/static_assert.h>
#include <aos/urpc.h>


#define URPC_MessageType_Error          URPC_MessageType_User0
#define URPC_MessageType_SocketOpen     URPC_MessageType_User1
#define URPC_MessageType_Send           URPC_MessageType_User2
#define URPC_MessageType_Receive        URPC_MessageType_User3
#define URPC_MessageType_SocketClose    URPC_MessageType_User4


struct udp_socket_common {
    uint16_t port;
};

struct udp_urpc_packet {
    uint32_t addr;
    uint16_t port;
    uint8_t payload[];
} __attribute__ ((__packed__));

#endif /* common_h */
