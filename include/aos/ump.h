//
//  ump.h
//  DoritOS
//

#ifndef ump_h
#define ump_h

#include <stdio.h>
#include <stdint.h>
#include <aos/aos.h>

#define UMP_BUF_SIZE           MON_URPC_SIZE
#define UMP_NUM_SLOTS          64
#define UMP_SLOT_DATA_BYTES    63

#define UMP_BSP_BUF_SELECT     0
#define UMP_APP_BUF_SELECT     1

#define UMP_CLIENT_BUF_SELECT     0
#define UMP_SERVER_BUF_SELECT     1


// UMP message types
#define UMP_MessageType_Bootinfo            0
#define UMP_MessageType_Spawn               1
#define UMP_MessageType_SpawnAck            2
#define UMP_MessageType_TerminalGetChar     3
#define UMP_MessageType_TerminalGetCharAck  4
#define UMP_MessageType_TerminalPutChar     5
#define UMP_MessageType_TerminalPutCharAck  6
#define UMP_MessageType_RegisterProcess     7
#define UMP_MessageType_RegisterProcessAck  8
#define UMP_MessageType_UrpcBindRequest     9
#define UMP_MessageType_UrpcBindAck         10
#define UMP_MessageType_DeregisterForward   11

#define UMP_MessageType_User0  32
#define UMP_MessageType_User1  33
#define UMP_MessageType_User2  34
#define UMP_MessageType_User3  35
#define UMP_MessageType_User4  36
#define UMP_MessageType_User5  37
#define UMP_MessageType_User6  38
#define UMP_MessageType_User7  39
#define UMP_MessageType_User8  40
#define UMP_MessageType_User9  41
#define UMP_MessageType_User10 42
#define UMP_MessageType_User11 43
#define UMP_MessageType_User12 44
#define UMP_MessageType_User13 45
#define UMP_MessageType_User14 46
#define UMP_MessageType_User15 47

// UMP message types type
typedef uint8_t ump_msg_type_t;


struct ump_chan {
    struct frame_identity fi;
    struct ump_buf *buf;
    uint8_t buf_select;         // Buffer for this process
    uint8_t tx_counter;
    uint8_t rx_counter;
    uint8_t ack_counter;
};


struct ump_buf_header {
    uint8_t tx_counter;
    uint8_t rx_counter;
    uint8_t ack_counter;
    char RESERVED[61];      // Make sure the size of the struct is 64 bytes
};

struct ump_slot {
    char data[63];
    ump_msg_type_t msg_type     : 6;
    uint8_t last                : 1;
    uint8_t valid               : 1;
};

struct ump_buf {
    struct ump_slot slots[UMP_NUM_SLOTS];
};


// Initialize a UMP frame
void ump_chan_init(struct ump_chan *chan, uint8_t buf_select);

// Send a buffer of at most UMP_SLOT_DATA_BYTES bytes on the UMP channel
errval_t ump_send_one(struct ump_chan *chan, const void *buf, size_t size,
                       ump_msg_type_t msg_type, uint8_t last);

// Send a buffer on the UMP channel
errval_t ump_send(struct ump_chan *chan, const void *buf, size_t size,
                   ump_msg_type_t msg_type);

// Receive a buffer of UMP_SLOT_DATA_BYTES bytes on the UMP channel
errval_t ump_recv_one(struct ump_chan *chan, void *buf,
                       ump_msg_type_t* msg_type, uint8_t *last);

// Receive a buffer of `size` bytes on the UMP channel
errval_t ump_recv(struct ump_chan *chan, void **buf, size_t *size,
                   ump_msg_type_t* msg_type);


// Block until receive a buffer of `size` bytes on the UMP channel
void ump_recv_blocking(struct ump_chan *chan, void **buf, size_t *size,
                    ump_msg_type_t *msg_type);

#endif /* ump_h */
