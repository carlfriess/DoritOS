//
//  main.c
//  DoritOS
//
//  Created by Carl Friess on 15/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <stdio.h>

#include <aos/aos.h>
#include <aos/aos_rpc.h>

#include <netutil/user_serial.h>

#include <maps/omap44xx_map.h>

#include "slip.h"
#include "ip.h"
#include "udp.h"


// Serial receive handler
void serial_input(uint8_t *buf, size_t len) {
    
    slip_recv(buf, len);
    
}

int main(int argc, char *argv[]) {
    
    errval_t err;
    
    // Get the device frame capability
    struct capref devcap;
    err = aos_rpc_get_device_cap(get_init_rpc(), OMAP44XX_MAP_L4_PER_UART4,
                                 OMAP44XX_MAP_L4_PER_UART4_SIZE, &devcap);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "getting device cap");
        return err;
    }
    
    // Get the frame identity
    struct frame_identity id;
    err = invoke_frame_identify(devcap, &id);
    assert(err_is_ok(err));
    
    debug_printf("UART device at %p (%u bytes)\n", (lpaddr_t)id.base, (uint32_t)id.bytes);
    
    // Map the device region uncachable
    void *devmem;
    err = paging_map_frame_attr(get_current_paging_state(), &devmem,
                                OMAP44XX_MAP_L4_PER_UART4_SIZE, devcap,
                                VREGION_FLAGS_READ_WRITE_NOCACHE, NULL, NULL);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "mapping device cap");
        return err;
    }
    
    // Initialize the serial interface
    err = serial_init((lvaddr_t) devmem, UART4_IRQ);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "initializing serial");
        return err;
    }
    
    // Initialize the SLIP parser
    if (slip_init()) {
        debug_printf("Error in slip_init()\n");
        return -1;
    }
    
    // Initialite the UDP module
    udp_init();
    
    
    // Set the IP address for this host
    ip_set_ip_address(10, 0, 2, 1);
    
    
    domainid_t pid;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "udp_echo", 0, &pid);
    
    
    // MARK: - Message handling
    
    // Get default waitset
    struct waitset *default_ws = get_default_waitset();
    
    // Register Event Queue for UDP
    udp_register_event_queue(default_ws);
    
    // Dispatch events
    while (true) {
        event_dispatch(default_ws);
    }
    
}
