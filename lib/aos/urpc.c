//
//  urpc.c
//  DoritOS
//


#include <aos/capabilities.h>
#include <machine/atomic.h>

#include "aos/urpc.h"

// Initialize a URPC channel
void urpc_chan_init(struct urpc_chan *chan) {
    
    // Get the headers
    struct urpc_buf_header *rx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_RX_OFFSET);
    struct urpc_buf_header *tx_header = (struct urpc_buf_header *) (chan->buf + URPC_BSP_TX_OFFSET);
    
    // Set the counters to zero
    rx_header->seq_counter = 0;
    rx_header->ack_counter = 0;
    tx_header->seq_counter = 0;
    tx_header->ack_counter = 0;
    
}

// On BSP: Get pointer to the memory region, where to write the message to be sent
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
    
}
