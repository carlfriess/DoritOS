#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/ump.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>

#define URPC_MessageType_Ping   URPC_MessageType_User0
#define URPC_MessageType_Pong   URPC_MessageType_User1

int main(int argc, char *argv[]) {
    
    errval_t err;
    
    // Spawn a client
    domainid_t pid;
    aos_rpc_process_spawn(aos_rpc_get_init_channel(), "bind_client", 0, &pid);
    
    // Accept a binding request from a client
    struct urpc_chan chan;
    err = urpc_accept_blocking(&chan);
    assert(err_is_ok(err));

    // Variable declarations
    uint32_t counter;
    size_t size;
    void *ptr;
    urpc_msg_type_t msg_type;

    while (true) {
        
        // Receive counter value
        urpc_recv_blocking(&chan, &ptr, &size, &msg_type);
        assert(msg_type == URPC_MessageType_Ping);

        // Deref, increment and assign counter
        counter = (*((uint32_t *) ptr)) + 1;

        if (counter % 100000 == 1) {
            printf("PING: %d\n", counter);
        }
        
        // Free the message
        free(ptr);

        // Send counter value
        urpc_send(&chan, (void *) &counter, sizeof(uint32_t), URPC_MessageType_Pong);
        
    }
}
