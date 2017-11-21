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

struct urpc_spaw_response {
    errval_t err;
    domainid_t pid;
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


#endif /* urpc_h */
