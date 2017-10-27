#include <aos/aos.h>
#include <aos/process.h>

static struct process_info *process_list = NULL;


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

// Returns the process information for a specific PID
struct process_info *process_info_for_pid(domainid_t pid) {
    assert(pid != 0);
    for (struct process_info *node = process_list; node != NULL; node = node->next) {
        if (node->pid == pid) {
            return node;
        }
    }
    return NULL;
}

// Returns the name of the process with the PID
char *process_name_for_pid(domainid_t pid)  {
    if (pid == 0) { return "init"; }
    struct process_info *pi = process_info_for_pid(pid);
    return pi ? pi->name : "";
}

// Creates an array of all current PIDs and returns the count
size_t get_all_pids(domainid_t **ret_list)  {
    
    // Count the running processes
    size_t count = 0;
    for (struct process_info *pi = process_list; pi != NULL; pi = pi->next) {
        count++;
    }
    
    // Allocate space for the return array
    *ret_list = malloc(sizeof(domainid_t) * count);
    
    // Walk the list and copy the PIDs into the array
    size_t index = 0;
    for (struct process_info *pi = process_list; pi != NULL; pi = pi->next) {
        *((*ret_list) + index++) = pi->pid;
    }
    
    return count;
    
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
