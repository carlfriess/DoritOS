//
//  multicore.h
//  DoritOS
//

#ifndef multicore_h
#define multicore_h

#include <stdio.h>
#include <aos/aos.h>

errval_t boot_core(coreid_t core_id, void **urpc_frame);

#endif /* multicore_h */
