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

    domainid_t pid = 0;

    // Find bind_server's pid
    err = aos_rpc_process_get_pid_by_name("bind_server", &pid);
    assert(err_is_ok(err));
    printf("Found bind_server, PID: %d\n", pid);

    
    // Bind to the server
    struct urpc_chan chan;
    err = urpc_bind(pid, &chan, false);
    assert(err_is_ok(err));

    // Variable declarations
    uint32_t counter = 0;
    size_t size;
    void *ptr;
    urpc_msg_type_t msg_type;

    while (true) {
        
        // Send counter value
        urpc_send(&chan, (void *) &counter, sizeof(uint32_t), URPC_MessageType_Ping);

        // Receive counter value
        urpc_recv_blocking(&chan, &ptr, &size, &msg_type);
        assert(msg_type == URPC_MessageType_Pong);

        // Deref, increment and assign counter
        counter = (*((uint32_t *) ptr)) + 1;
        
        // Free the message
        free(ptr);

        if (counter % 100000 == 0) {
            printf("PONG: %d\n", counter);
        }
        
    }

}
