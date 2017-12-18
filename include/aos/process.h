#ifndef _AOS_PROCESS_H_
#define _AOS_PROCESS_H_

#include "aos/lmp_chan.h"

struct process_info {
    struct process_info *next;
    domainid_t pid;
    coreid_t core_id;
    char *name;
    struct capref *dispatcher_cap;
    struct lmp_chan *lc;
};

void process_register(struct process_info *pi);

struct process_info *process_info_for_pid(domainid_t pid);
domainid_t process_pid_for_lmp_chan(struct lmp_chan *lc);
char *process_name_for_pid(domainid_t pid);
size_t get_all_pids(domainid_t *ret_list);
void print_process_list(void);

#endif
