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
    
    // Set the index of the next slot to use for sending
    chan->tx_counter = (chan->tx_counter + 1) % URPC_NUM_SLOTS;
    
    // Make sure there is space in the ring buffer and wait otherwise
    while (tx_buf->slots[chan->tx_counter].valid) ;
    
    // Memory barrier
    dmb();
    
    // Copy data to the slot
    memcpy(tx_buf->slots[chan->tx_counter].data, buf, size);
    tx_buf->slots[chan->tx_counter].msg_type = msg_type;
    tx_buf->slots[chan->tx_counter].last = last;
    
    // Memory and instruction barrier
    dmb();
    
    // Mark the message as valid
    tx_buf->slots[next_slot_index].valid = 1;
    
    return ERR_SYS_OK;
    
}

// Send a buffer on the URPC channel
errval_t urpc_send(struct urpc_chan *chan, void *buf, size_t size,
                   urpc_msg_type_t msg_type) {
    
    errval_t err = ERR_SYS_OK;
    
    while (size > 0) {
        
        size_t msg_size = MIN(size, URPC_SLOT_DATA_BYTES);
        
        err = urpc_send(chan,
                        buf,
                        msg_type,
                        msg_size < URPC_SLOT_DATA_BYTES);
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
    memcpy(*buf, rx_buf->slots[chan->rx_counter].data, URPC_SLOT_DATA_BYTES);
    *msg_type = rx_buf->slots[chan->rx_counter].msg_type;
    *last = rx_buf->slots[chan->rx_counter].last;

    // Memory barrier
    //dmb();
    
    // Set the index of the next slot to read
    chan->rx_counter = (chan->rx_counter + 1) % URPC_NUM_SLOTS;
    
    // Memory and instruction barrier
    dmb();
    
    // Mark the message as invalid
    tx_buf->slots[next_slot_index].valid = 0;
    
    return ERR_SYS_OK;
    
}

// Receive a buffer of at most `max_size` bytes on the URPC channel
errval_t urpc_recv(struct urpc_chan *chan, void *buf, size_t max_size,
                   urpc_msg_type_t* msg_type) {
    
    // Check that output buffer size is sufficient
    assert(max_size >= URPC_SLOT_DATA_BYTES);
    
    errval_t err = SYS_ERR_OK;
    
    urpc_msg_type_t msg_type;
    uint8_t last;
    size_t msg_size = 0;
    
    // Check that we have received an initial message
    err = urpc_recv_one(chan, buf, &msg_type, &last);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Increase the received message size
    msg_size += URPC_SLOT_DATA_BYTES;
    
    // Loop until we received the final message
    while (!last) {
        
        // Check we still have enough space in the output buffer
        if (msg_size + URPC_SLOT_DATA_BYTES > max_size) {
            return UMP_FRAME_OVERFLOW;
        }
        
        urpc_msg_type_t this_msg_type;
        
        // Receive the next message
        err = urpc_recv_one(chan, buf + msg_size, &this_msg_type, &last);
        if (err == LIB_ERR_NO_UMP_MSG) {
            continue;
        }
        else if (err_is_fail(err)) {
            return err;
        }
        
        // Check the message types are consisten
        assert(this_msg_type == msg_type);
        
        // Increase the received message size
        msg_size += URPC_SLOT_DATA_BYTES;
        
    }
    
    return err;
    
}


/* MARK: - ===== LEGACY CODE ===== */

/*// On BSP: Get pointer to the memory region, where to write the message to be sent
void *urpc_get_send_to_app_buf(struct urpc_chan *chan) {
    
    // Get the headers
    struct urpc_buf_header *rx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_RX_OFFSET);
    struct urpc_buf_header *tx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_TX_OFFSET);
    
    // Wait until the last message has been acknowledged
    do {
        
        // Invalidate the cache
        sys_armv7_cache_invalidate((void *) ((uint32_t) chan->fi.base + URPC_BSP_RX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_BSP_RX_OFFSET + URPC_BSP_RX_SIZE));
        
        // Memory barrier
        dmb();
        
    } while (rx_header->ack_counter != tx_header->seq_counter);
    
    return chan->buf + URPC_BSP_TX_OFFSET + sizeof(struct urpc_buf_header);
}

// On APP: Get pointer to the memory region, where to write the message to be sent
void *urpc_get_send_to_bsp_buf(struct urpc_chan *chan) {
    
    // Get the headers
    struct urpc_buf_header *rx_header = (struct urpc_buf_header *) (chan->buf + URPC_APP_RX_OFFSET);
    struct urpc_buf_header *tx_header = (struct urpc_buf_header *) (chan->buf + URPC_APP_TX_OFFSET);
    
    // Wait until the last message has been acknowledged
    do {
        
        // Invalidate the cache
        sys_armv7_cache_invalidate((void *) ((uint32_t) chan->fi.base + URPC_APP_RX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_APP_RX_OFFSET + URPC_APP_RX_SIZE));
        
        // Memory barrier
        dmb();
        
    } while (rx_header->ack_counter != tx_header->seq_counter);
    
    return chan->buf + URPC_APP_TX_OFFSET + sizeof(struct urpc_buf_header);
    
}

// Send a message to the APP cpu
void urpc_send_to_app(struct urpc_chan *chan) {
    
    // Get the header
    struct urpc_buf_header *header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_TX_OFFSET);
    
    // Increment the sequence counter
    header->seq_counter++;
    
    // Memory barrier
    dmb();
    
    // Clean (flush) the cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) chan->fi.base + URPC_BSP_TX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_BSP_TX_OFFSET + URPC_BSP_TX_SIZE));
    
}

// Send a message to the BSP cpu
void urpc_send_to_bsp(struct urpc_chan *chan) {
    
    // Get the header
    struct urpc_buf_header *header = (struct urpc_buf_header *) (chan->buf + URPC_APP_TX_OFFSET);
    
    // Increment the counter
    header->seq_counter++;
    
    // Memory barrier
    dmb();
    
    // Clean (flush) the cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) chan->fi.base + URPC_APP_TX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_APP_TX_OFFSET + URPC_APP_TX_SIZE));
    
}

// Check for and receive a message from the APP cpu (non-blocking)
void *urpc_recv_from_app(struct urpc_chan *chan) {
    
    static uint32_t last_counter = 0;
    
    // Get the header
    struct urpc_buf_header *rx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_RX_OFFSET);
    
    // Invalidate the cache
    sys_armv7_cache_invalidate((void *) ((uint32_t) chan->fi.base + URPC_BSP_RX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_BSP_RX_OFFSET + URPC_BSP_RX_SIZE));
    
    // Memory barrier
    dmb();
    
    // Check if a new message has been received
    if (rx_header->seq_counter == last_counter + 1) {
        
        last_counter++;
        
        return chan->buf + URPC_BSP_RX_OFFSET + sizeof(struct urpc_buf_header);
        
    }
    
    return NULL;
    
}

// Check for and receive a message from the BSP cpu (non-blocking)
void *urpc_recv_from_bsp(struct urpc_chan *chan) {
    
    static uint32_t last_counter = 0;
    
    // Get the header
    struct urpc_buf_header *rx_header = (struct urpc_buf_header *) (chan->buf + URPC_APP_RX_OFFSET);
    
    // Invalidate the cache
    sys_armv7_cache_invalidate((void *) ((uint32_t) chan->fi.base + URPC_APP_RX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_APP_RX_OFFSET + URPC_APP_RX_SIZE));
    
    // Memory barrier
    dmb();
    
    // Check if a new message has been received
    if (rx_header->seq_counter == last_counter + 1) {
        
        last_counter++;
        
        return chan->buf + URPC_APP_RX_OFFSET + sizeof(struct urpc_buf_header);
        
    }
    
    return NULL;
    
}

// Acknowledge the last message received from the APP
void urpc_ack_recv_from_app(struct urpc_chan *chan) {
    
    // Get the headers
    struct urpc_buf_header *tx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_TX_OFFSET);
    
    // Increment the ACK counter
    tx_header->ack_counter++;
    
    // Memory barrier
    dmb();
    
    // Clean (flush) the cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) chan->fi.base + URPC_BSP_TX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_BSP_TX_OFFSET + URPC_BSP_TX_SIZE));
    
}

// Acknowledge the last message received from the BSP
void urpc_ack_recv_from_bsp(struct urpc_chan *chan) {
    
    // Get the header
    struct urpc_buf_header *tx_header = (struct urpc_buf_header *) (chan->buf + URPC_APP_TX_OFFSET);
    
    // Increment the ACK counter
    tx_header->ack_counter++;
    
    // Memory barrier
    dmb();
    
    // Clean (flush) the cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) chan->fi.base + URPC_APP_TX_OFFSET), (void *) ((uint32_t) chan->fi.base + URPC_APP_TX_OFFSET + URPC_APP_TX_SIZE));
    
}*/
