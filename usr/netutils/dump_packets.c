//
//  dump_packets.c
//  DoritOS
//
//  Created by Carl Friess on 21/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <aos/aos.h>

#include <net/udp_socket.h>


static void usage(void) {
    printf("Usage: dump_packets {off|on}\n");
}

int main(int argc, char *argv[]) {
    
    errval_t err;
    
    if (argc != 2) {
        usage();
        return 0;
    }
    
    bool dump;
    
    if (!strcmp(argv[1], "off")) {
        dump = false;
    }
    else if (!strcmp(argv[1], "on")) {
        dump = true;
    }
    else {
        usage();
        return 0;
    }
    
    struct udp_socket s;
    
    err = socket(&s, 0);
    if (err_is_fail(err)) {
        printf("%s\n", err_getstring(err));
        return 1;
    }
    
    err = urpc_send(&s.chan,
                    (void *) &dump,
                    sizeof(bool),
                    URPC_MessageType_DumpPackets);
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
