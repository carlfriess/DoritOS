#include <aos/aos.h>
#include <aos/process.h>

//static size_t pid = 0;
struct process_info *process_list = NULL;


void process_register(struct process_info *pi) {
    
    // Counter for assigning new PIDs
    static domainid_t pid_counter = 1;
    
    // Set the PID for the new process
    pi->pid = pid_counter++;
    
    // Set the next point to NULL just in case
    pi->next = NULL;
    
    // Insert at the end of the list
    struct process_info **node;
    for (node = &process_list; *node != NULL; node = &((*node)->next));
    *node = pi;
}

void print_process_list(void) {
    int counter = 0;
    
    struct process_info *i;
    debug_printf("-----------------------------\n");
    debug_printf("Currently running processes:\n");
    debug_printf("\t%3d\t%s\n", 0, "init");
    for (i = process_list; i != NULL; i = i->next) {
        debug_printf("\t%3d\t%s\n", i->pid, i->name);
        counter++;
    }
    debug_printf("Total number of processes: %d\n", counter);
    debug_printf("-----------------------------\n");
}
