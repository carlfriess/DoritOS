//
//  main.c
//  DoritOS
//
//  Created by Carl Friess on 15/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>

#include <aos/aos.h>

#include <net/udp_socket.h>


int main(int argc, char *argv[]) {
    
    errval_t err;
    
    struct udp_socket s;
    
    err = socket(&s, 80);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return 1;
    }
    
    debug_printf("Socket open!\n");
    
    while (true) {
        
        char buf[256];
        uint32_t from_addr;
        uint16_t from_port;
        size_t ret_size;
        
        err = recvfrom(&s,
                       (void *) buf,
                       sizeof(buf),
                       &ret_size,
                       &from_addr,
                       &from_port);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return 1;
        }
        
        err = sendto(&s, buf, ret_size, from_addr, from_port);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return 1;
        }
        
        printf("Echoed: %s\n", buf);
        
    }
    
    return 0;
    
}
