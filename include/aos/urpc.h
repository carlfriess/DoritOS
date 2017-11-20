//
//  urpc.h
//  DoritOS
//

#ifndef urpc_h
#define urpc_h

#include <stdio.h>
#include <stdint.h>
#include <aos/aos.h>

#define URPC_BUF_SIZE           MON_URPC_SIZE
#define URPC_NUM_SLOTS          64
#define URPC_SLOT_DATA_BYTES    63

#define URPC_BSP_BUF_SELECT     0
#define URPC_APP_BUF_SELECT     1

/*#define URPC_APP_RX_OFFSET  0
#define URPC_APP_RX_SIZE    URPC_BUF_SIZE/2
#define URPC_BSP_RX_OFFSET  URPC_BUF_SIZE/2
#define URPC_BSP_RX_SIZE    URPC_BUF_SIZE/2

#define URPC_APP_TX_OFFSET  URPC_BSP_RX_OFFSET
#define URPC_APP_TX_SIZE    URPC_BSP_RX_SIZE
#define URPC_BSP_TX_OFFSET  URPC_APP_RX_OFFSET
#define URPC_BSP_TX_SIZE    URPC_APP_RX_SIZE*/


// URPC message types
#define URPC_MessageType_Bootinfo   0
#define URPC_MessageType_Spawn      1
#define URPC_MessageType_SpawnAck   2


// URPC message types type
typedef uint8_t urpc_msg_type_t;


struct urpc_chan {
    struct frame_identity fi;
    struct urpc_buf *buf;
    uint8_t buf_select;         // Buffer for this process
    uint8_t tx_counter;
    uint8_t rx_counter;
    uint8_t ack_counter;
};


struct urpc_buf_header {
    uint8_t tx_counter;
    uint8_t rx_counter;
    uint8_t ack_counter;
    char RESERVED[61];      // Make sure the size of the struct is 64 bytes
};

struct urpc_slot {
    char data[63];
    urpc_msg_type_t msg_type    : 6;
    uint8_t last                : 1;
    uint8_t valid               : 1;
};

struct urpc_buf {
    struct urpc_slot slots[URPC_NUM_SLOTS];
};


struct module_frame_identity {
    struct frame_identity fi;
    cslot_t slot;
};

struct urpc_bi_caps {
    struct frame_identity bootinfo;
    struct frame_identity mmstrings_cap;
    size_t num_modules;
    struct module_frame_identity modules[];
};


// Initialize a URPC frame
void urpc_chan_init(struct urpc_chan *chan, uint8_t buf_select);

// Send a buffer of at most URPC_SLOT_DATA_BYTES bytes on the URPC channel
errval_t urpc_send_one(struct urpc_chan *chan, void *buf, size_t size,
                       urpc_msg_type_t msg_type, uint8_t last);

// Send a buffer on the URPC channel
errval_t urpc_send(struct urpc_chan *chan, void *buf, size_t size,
                   urpc_msg_type_t msg_type);

// Receive a buffer of URPC_SLOT_DATA_BYTES bytes on the URPC channel
errval_t urpc_recv_one(struct urpc_chan *chan, void *buf,
                       urpc_msg_type_t* msg_type, uint8_t *last);

// Receive a buffer of `size` bytes on the URPC channel
errval_t urpc_recv(struct urpc_chan *chan, void **buf, size_t *size,
                   urpc_msg_type_t* msg_type);

/*// On BSP: Get pointer to the memory region, where to write the message to be sent
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

// Acknowledge the last message received from the APP
void urpc_ack_recv_from_app(struct urpc_chan *chan);

// Acknowledge the last message received from the BSP
void urpc_ack_recv_from_bsp(struct urpc_chan *chan);

inline void *urpc_get_send_buf(struct urpc_chan *chan) {
    if (disp_get_core_id() == 0) {
        return urpc_get_send_to_app_buf(chan);
    }
    else {
        return urpc_get_send_to_bsp_buf(chan);
    }
}

inline void urpc_send(struct urpc_chan *chan) {
    if (disp_get_core_id() == 0) {
        urpc_send_to_app(chan);
    }
    else {
        urpc_send_to_bsp(chan);
    }
}

inline void *urpc_recv(struct urpc_chan *chan) {
    if (disp_get_core_id() == 0) {
        return urpc_recv_from_app(chan);
    }
    else {
        return urpc_recv_from_bsp(chan);
    }
}

inline void urpc_ack_recv(struct urpc_chan *chan) {
    if (disp_get_core_id() == 0) {
        urpc_ack_recv_from_app(chan);
    }
    else {
        urpc_ack_recv_from_bsp(chan);
    }
}*/

#endif /* urpc_h */
