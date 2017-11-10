//
//  multicore.h
//  DoritOS
//

#ifndef multicore_h
#define multicore_h

#include <stdio.h>
#include <aos/aos.h>

#define URPC_BUF_SIZE       MON_URPC_SIZE

#define URPC_APP_RX_OFFSET  0
#define URPC_APP_RX_SIZE    URPC_BUF_SIZE/2
#define URPC_BSP_RX_OFFSET  URPC_BUF_SIZE/2
#define URPC_BSP_RX_SIZE    URPC_BUF_SIZE/2

#define URPC_APP_TX_OFFSET  URPC_BSP_RX_OFFSET
#define URPC_APP_TX_SIZE    URPC_BSP_RX_SIZE
#define URPC_BSP_TX_OFFSET  URPC_APP_RX_OFFSET
#define URPC_BSP_TX_SIZE    URPC_APP_RX_SIZE


struct urpc_chan {
    struct frame_identity fi;
    void *buf;
};


// Boot the the core with the ID core_id
errval_t boot_core(coreid_t core_id, struct urpc_chan *chan);

// Initialize a URPC frame
void urpc_chan_init(struct urpc_chan *chan);

// On BSP: Get pointer to the memory region, where to write the message to be sent
void *urpc_get_send_to_app_buf(struct urpc_chan *chan);

// On APP: Get pointer to the memory region, where to write the message to be sent
void *urpc_get_send_to_bsp_buf(struct urpc_chan *chan);

// Send a message to the APP cpu
void urpc_send_to_app(struct urpc_chan *chan);

// Send a message to the BSP cpu
void urpc_send_to_bsp(struct urpc_chan *chan);

// Check for and receive a message from the APP cpu (non-blocking)
void *urpc_recv_from_app(struct urpc_chan *chan);

// Check for and receive a message from the BSP cpu (non-blocking)
void *urpc_recv_from_bsp(struct urpc_chan *chan);


#endif /* multicore_h */
