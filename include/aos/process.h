#ifndef _AOS_PROCESS_H_
#define _AOS_PROCESS_H_

#include "aos/lmp_chan.h"

struct process_info *process_list;

struct process_info {
    struct process_info *next;
    domainid_t pid;
    char *name;
    struct capref *dispatcher_cap;
    struct lmp_chan *lc;
};

void process_register(struct process_info *pi);

void print_process_list(void);

#endif
