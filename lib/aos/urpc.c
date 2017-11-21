//
//  urpc.c
//  DoritOS
//

#include <string.h>

#include <aos/capabilities.h>
#include <machine/atomic.h>

#include "aos/urpc.h"

// Initialize a URPC channel
void urpc_chan_init(struct urpc_chan *chan, uint8_t buf_select) {
    
    // Set buffer selector
    assert(buf_select < 2);
    chan->buf_select = buf_select;
    
    // Set the counters to zero
    chan->tx_counter = 0;
    chan->rx_counter = 0;
    chan->ack_counter = 0;
    
}

// Send a buffer of at most URPC_SLOT_DATA_BYTES bytes on the URPC channel
errval_t urpc_send_one(struct urpc_chan *chan, void *buf, size_t size,
                       urpc_msg_type_t msg_type, uint8_t last) {
    
    // Check for invalid sizes
    //assert(size <= URPC_SLOT_DATA_BYTES);
    if (size > URPC_SLOT_DATA_BYTES) {
        return LIB_ERR_UMP_BUFSIZE_INVALID;
    }
    
    // Get the correct URPC buffer
    struct urpc_buf *tx_buf = chan->buf + chan->buf_select;
    
    // Make sure there is space in the ring buffer and wait otherwise
    while (tx_buf->slots[chan->tx_counter].valid) ;
    
    // Memory barrier
    dmb();
    
    // Copy data to the slot
    memcpy(tx_buf->slots[chan->tx_counter].data, buf, size);
    tx_buf->slots[chan->tx_counter].msg_type = msg_type;
    tx_buf->slots[chan->tx_counter].last = last;
    
    // Memory barrier
    dmb();
    
    // Mark the message as valid
    tx_buf->slots[chan->tx_counter].valid = 1;
    
    // Memory barrier
    //  TODO: Is this necessary?
    dmb();
    
    // Set the index of the next slot to use for sending
    chan->tx_counter = (chan->tx_counter + 1) % URPC_NUM_SLOTS;
    
    return SYS_ERR_OK;
    
}

// Send a buffer on the URPC channel
errval_t urpc_send(struct urpc_chan *chan, void *buf, size_t size,
                   urpc_msg_type_t msg_type) {
    
    errval_t err = SYS_ERR_OK;
    
    while (size > 0) {
        
        size_t msg_size = MIN(size, URPC_SLOT_DATA_BYTES);
        
        // Send fragment of entire message
        err = urpc_send_one(chan,
                            buf,
                            msg_size,
                            msg_type,
                            size <= URPC_SLOT_DATA_BYTES);
        if (err_is_fail(err)) {
            return err;
        }
        
        buf += msg_size;
        size -= msg_size;
        
    }
    
    return err;
    
}

// Receive a buffer of URPC_SLOT_DATA_BYTES bytes on the URPC channel
errval_t urpc_recv_one(struct urpc_chan *chan, void *buf,
                       urpc_msg_type_t* msg_type, uint8_t *last) {
    
    // Get the correct URPC buffer
    struct urpc_buf *rx_buf = chan->buf + !chan->buf_select;
    
    // Check if there is a new message
    if (!rx_buf->slots[chan->rx_counter].valid) {
        return LIB_ERR_NO_UMP_MSG;
    }
    
    // Copy data from the slot
    memcpy(buf, rx_buf->slots[chan->rx_counter].data, URPC_SLOT_DATA_BYTES);
    *msg_type = rx_buf->slots[chan->rx_counter].msg_type;
    *last = rx_buf->slots[chan->rx_counter].last;

    // Memory barrier
    dmb();
    
    // Mark the message as invalid
    rx_buf->slots[chan->rx_counter].valid = 0;

    // Memory barrier
    dmb();
    
    // Set the index of the next slot to read
    chan->rx_counter = (chan->rx_counter + 1) % URPC_NUM_SLOTS;
    
    return SYS_ERR_OK;
    
}

// Receive a buffer of `size` bytes on the URPC channel
errval_t urpc_recv(struct urpc_chan *chan, void **buf, size_t *size,
                   urpc_msg_type_t* msg_type) {
    
    errval_t err = SYS_ERR_OK;
    
    uint8_t last;
    
    // Allocate memory for the first message
    *size = URPC_SLOT_DATA_BYTES;
    *buf = malloc(*size);
    if (*buf == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Check that we have received an initial message
    err = urpc_recv_one(chan, *buf, msg_type, &last);
    if (err_is_fail(err)) {
        free(*buf);
        return err;
    }
    
    // Loop until we received the final message
    while (!last) {
        
        urpc_msg_type_t this_msg_type;
        
        // Allocate more space for the next message
        *size += URPC_SLOT_DATA_BYTES;
        *buf = realloc(*buf, *size);
        if (*buf == NULL) {
            return LIB_ERR_MALLOC_FAIL;
        }
        
        // Receive the next message
        err = urpc_recv_one(chan,
                            *buf + *size - URPC_SLOT_DATA_BYTES,
                            &this_msg_type,
                            &last);
        if (err == LIB_ERR_NO_UMP_MSG) {
            continue;
        }
        else if (err_is_fail(err)) {
            free(*buf);
            return err;
        }
        
        // Check the message types are consisten
        assert(this_msg_type == *msg_type);
        
    }
    
    return err;
    
}
