#ifndef _AOS_PROCESS_H_
#define _AOS_PROCESS_H_

#include "aos/lmp_chan.h"

struct process_info {
    struct process_info *next;
    domainid_t pid;
    char *name;
    struct capref *dispatcher_cap;
    struct lmp_chan *lc;
};

void process_register(struct process_info *pi);

struct process_info *process_info_for_pid(domainid_t pid);
char *process_name_for_pid(domainid_t pid);
size_t get_all_pids(domainid_t *ret_list);
void print_process_list(void);

#endif
