//
//  urpc_protocol.h
//  DoritOS
//
//  Created by Carl Friess on 22/11/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef urpc_protocol_h
#define urpc_protocol_h


struct module_frame_identity {
    struct frame_identity fi;
    cslot_t slot;
};

struct urpc_bi_caps {
    struct frame_identity bootinfo;
    struct frame_identity mmstrings_cap;
    size_t num_modules;
    struct module_frame_identity modules[];
};

struct urpc_spaw_response {
    errval_t err;
    domainid_t pid;
};

struct urpc_process_register {
    coreid_t core_id;
    domainid_t pid;
    char name[];
};


#endif /* urpc_protocol_h */
