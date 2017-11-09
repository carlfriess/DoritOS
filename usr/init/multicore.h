//
//  multicore.h
//  DoritOS
//

#ifndef multicore_h
#define multicore_h

#include <stdio.h>
#include <aos/aos.h>

#define URPC_APP_RX_OFFSET  0
#define URPC_BSP_RX_OFFSET  BASE_PAGE_SIZE/2

#define URPC_APP_TX_OFFSET  URPC_BSP_RX_OFFSET
#define URPC_BSP_TX_OFFSET  URPC_APP_RX_OFFSET

// Boot the the core with the ID core_id
errval_t boot_core(coreid_t core_id, void **urpc_frame);

// Initialize a URPC frame
void urpc_frame_init(void *urpc_frame);

// On BSP: Get pointer to the memory region, where to write the message to be sent
void *get_urpc_send_to_app_buf(void *urpc_frame);

// On APP: Get pointer to the memory region, where to write the message to be sent
void *get_urpc_send_to_bsp_buf(void *urpc_frame);

// Send a message to the APP cpu
void send_to_app(void *urpc_frame);

// Send a message to the BSP cpu
void send_to_bsp(void *urpc_frame);

#endif /* multicore_h */
