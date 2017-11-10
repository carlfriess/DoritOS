#include <stdlib.h>
#include <aos/aos.h>
#include <aos/lmp.h>
#include <spawn/spawn.h>

#include "spawn_serv.h"

static struct urpc_chan *urpc_chan;

static errval_t request_remote_spawn(char *name, coreid_t coreid, domainid_t *pid) {
    
    // Get message buffer
    void *send_buf = urpc_get_send_buf(urpc_chan);
    
    // Set message type to spawn request
    *(enum urpc_msg_type *) send_buf = URPC_MessageType_Spawn;
    send_buf += sizeof(enum urpc_msg_type);
    
    // Copy name to message buffer
    strcpy((char *) send_buf, name);
    
    // Send request to spawn server on other core
    urpc_send(urpc_chan);
 
    // Waiting for response
    void *recv_buf;
    while (!(recv_buf = urpc_recv(urpc_chan)) || *(enum urpc_msg_type *) recv_buf != URPC_MessageType_SpawnAck);
    
    // Set pid of spawned process
    *pid = *(domainid_t *) (recv_buf + sizeof(enum urpc_msg_type) + sizeof(errval_t));
    
    // Return error code
    return *(errval_t *) (recv_buf + sizeof(enum urpc_msg_type));
    
}

static errval_t spawn_serv_handler(char *name, coreid_t coreid, domainid_t *pid) {
    errval_t err;
    
    
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
