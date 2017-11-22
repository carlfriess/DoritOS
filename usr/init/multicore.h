//
//  multicore.h
//  DoritOS
//

#ifndef multicore_h
#define multicore_h

#include <stdio.h>
#include <stdint.h>
#include <aos/aos.h>
#include <aos/urpc.h>
#include <aos/urpc_protocol.h>


// Boot the the core with the ID core_id
errval_t boot_core(coreid_t core_id, struct urpc_chan *chan);

// Forge all the needed module and RAM capabilites based on bootinfo
errval_t forge_module_caps(struct urpc_bi_caps *bi_frame_identities, coreid_t my_core_id);

#endif /* multicore_h */
