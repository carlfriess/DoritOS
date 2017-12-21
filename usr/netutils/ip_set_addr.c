//
//  ip_set_addr.c
//  DoritOS
//
//  Created by Carl Friess on 21/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <aos/aos.h>

#include <net/udp_socket.h>


static void usage(void) {
    printf("Usage: ip_set_addr ip\n");
}

int main(int argc, char *argv[]) {
    
    errval_t err;
    
    if (argc != 2) {
        usage();
        return 0;
    }
    
    int a, b, c, d;
    
    if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        usage();
        return 0;
    }
    
    uint8_t ip[4];
    ip[0] = a; ip[1] = b; ip[2] = c; ip[3] = d;
    
    struct udp_socket s;
    
    err = socket(&s, 0);
    if (err_is_fail(err)) {
        printf("%s\n", err_getstring(err));
        return 1;
    }
    
    err = urpc_send(&s.chan, (void *) ip, 4, URPC_MessageType_SetIPAddress);
    if (err_is_fail(err)) {
        printf("%s\n", err_getstring(err));
        return 1;
    }
    
    err = close(&s);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return 1;
    }
    
    return 0;
    
}
