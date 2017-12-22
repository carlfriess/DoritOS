//
//  udp_send.c
//  DoritOS
//
//  Created by Carl Friess on 22/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <aos/aos.h>

#include <net/udp_socket.h>


static void usage(void) {
    printf("Usage: udp_send ip port msg\n");
}

int main(int argc, char *argv[]) {
    
    errval_t err;
    
    if (argc != 4) {
        usage();
        return 0;
    }
    
    int a, b, c, d;
    
    if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        usage();
        return 0;
    }
    
    uint8_t ip[4];
    ip[3] = a; ip[2] = b; ip[1] = c; ip[0] = d;
    uint32_t *addr = (uint32_t *) ip;
    
    uint16_t port = atoi(argv[2]);
    
    struct udp_socket s;
    
    err = socket(&s, 0);
    if (err_is_fail(err)) {
        printf("%s\n", err_getstring(err));
        return 1;
    }
    
    err = sendto(&s,
                 (void *) argv[3],
                 strlen(argv[3]) + 1,
                 *addr,
                 port);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return 1;
    }
    
    err = close(&s);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return 1;
    }
    
    return 0;
    
}

