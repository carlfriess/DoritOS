#include <aos/aos.h>
#include <aos/lmp.h>
#include <spawn/spawn.h>

#include <stdlib.h>

#include "spawn_serv.h"

static errval_t spawn_serv_handler(char *name, coreid_t coreid, domainid_t *pid) {
    errval_t err;
    
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
 
    return err;
    
}

errval_t spawn_serv_init(void) {
    
    lmp_server_spawn_register_handler(spawn_serv_handler);
    
    return SYS_ERR_OK;
    
}
