#include <stdio.h>
#include <stdlib.h>

#include <aos/ump.h>
#include <aos/urpc.h>
#include <aos/aos_rpc.h>

int main(int argc, char *argv[]) {

    struct aos_rpc *rpc_chan = aos_rpc_get_init_channel();

    domainid_t pid = 0;

    // Find bind_server's pid
    {
        struct domainid_t *pids;
        size_t num_pids;

        // TODO: ERR
        aos_rpc_process_get_all_pids(rpc_chan, &pids, &num_pids);

        char *name;

        // Iterate pids and compare names
        for (int i = 0; i < num_pids; i++) {
            // TODO: ERR
            aos_rpc_process_get_name(rpc_chan, pids[i], &name);
            if (!strcmp("bind_server", name)) {
                pid = pids[i];
                printf("Found bind_server, PID: %d", pid);
                free(name);
                break;
            }
            free(name);
        }

        if (pid == 0) {
            USER_PANIC("bind_server NOT FOUND");
        }
    }

    struct ump_chan chan;
    // TODO: ERR
    urpc_bind(pid, &chan);


    // Variable declarations
    uint32_t counter = 0;
    size_t size;
    void *ptr;
    ump_msg_type_t msg_type;

    /*
        Kinda missed what you were talking about with the urpc_handle_lmp_bind_request() so imma just leave it here.
    */

    while (true) {
        // Send counter value
        ump_send(&chan, (void *) &counter, sizeof(uint32_t), UMP_MessageType_Ping);

        // Receive counter value
        ump_recv_blocking(&chan, &ptr, &size, &msg_type);
        assert(msg_type == UMP_MessageType_Pong);

        // Deref, increment and assign counter
        counter = (*((uint32_t *) ptr))++;

        printf("PONG: %d", counter);
    }

}