#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    struct ump_chan *chan;

    // Do your thang
    // urpc_listen(&chan);

    // Variable declarations
    uint32_t counter;
    size_t size;
    void *ptr;
    ump_msg_type_t msg_type;

    /*
        Also do the urpc_handle_lmp_bind_request() thingy.
    */

    while (true) {
        // Receive counter value
        ump_recv_blocking(&chan, &ptr, &size, &msg_type);
        assert(msg_type == UMP_MessageType_Ping);

        // Deref, increment and assign counter
        counter = (*((uint32_t *) ptr))++;

        printf("PING: %d", counter);

        // Send counter value
        ump_send(&chan, (void *) &counter, sizeof(uint32_t), UMP_MessageType_Pong);
    }
}