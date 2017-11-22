#include <stdlib.h>
#include <aos/aos.h>
#include <aos/lmp.h>
#include <spawn/spawn.h>
#include <aos/urpc_protocol.h>

#include "spawn_serv.h"

static struct urpc_chan *urpc_chan;

static errval_t request_remote_spawn(char *name, coreid_t coreid, domainid_t *pid) {
    
    errval_t err = SYS_ERR_OK;
    
    // Send request to spawn server on other core
    err = urpc_send(urpc_chan, name, strlen(name) + 1, URPC_MessageType_Spawn);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Waiting for response
    size_t retsize;
    urpc_msg_type_t msg_type;
    
    struct urpc_spaw_response *recv_buf;

    // Repeat until a message is received
    do {
        
        // Receive response of spawn server and save it in recv_buf
        err = urpc_recv(urpc_chan, (void **) &recv_buf, &retsize, &msg_type);
        
    } while (err == LIB_ERR_NO_UMP_MSG);
    
    // Check that receive was successful
    if (err_is_fail(err)) {
        return err;
    }
    
    // TODO: Handle incorrect URPC_MessageType gracefully
    assert(msg_type == URPC_MessageType_SpawnAck);
    
    // Set pid of spawned process
    *pid = recv_buf->pid;
    
    // Returned error code
    err = recv_buf->err;
    
    // Free memory of recv_buf
    free(recv_buf);
    
    // Return status
    return err;
    
}

errval_t spawn_serv_handler(char *name, coreid_t coreid, domainid_t *pid) {
    errval_t err;
    
    // Check if spawn request for this core
    if (coreid != disp_get_core_id()) {
        
        return request_remote_spawn(name, coreid, pid);
    
    }
    
    // Allocate spawninfo
    struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));
    if (si == NULL) {
        // Free spawn info
        free(si);
        return LIB_ERR_MALLOC_FAIL;
    }
    
    // Spawn memeater
    err = spawn_load_by_name(name, si);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        // Free spawn info
        free(si);
        return err;
    }
    
    // Return the new process id
    *pid = si->pi->pid;
    
    // Free the process info for memeater
    free(si);
    
    print_process_list();
 
    return err;
    
}

errval_t spawn_serv_init(struct urpc_chan *chan) {
    
    urpc_chan = chan;
    
    lmp_server_spawn_register_handler(spawn_serv_handler);
    
    return SYS_ERR_OK;
    
}
