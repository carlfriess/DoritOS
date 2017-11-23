#include <aos/aos.h>
#include <aos/urpc.h>
#include <aos/process.h>

static struct process_info *process_list = NULL;


void process_register(struct process_info *pi) {
    
    // Counter for assigning new PIDs
    static domainid_t pid_counter = 1;
    
    if (disp_get_core_id() == 0) {
        
        // Set the PID for the new process
        pi->pid = pid_counter++;
        
    }
    else {
        
        // Register process with init.0
        urpc_process_register(pi);
        
    }
    
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
//  ATTENTION: ret_list must be preallocated space
size_t get_all_pids(domainid_t *ret_list)  {
    
    size_t count = 1;

    // Add init
    *(ret_list++) = 0;
    
    // Walk the list and copy the PIDs into the array
    for (struct process_info *pi = process_list; pi != NULL; pi = pi->next) {
        *ret_list = pi->pid;
        ret_list++;
        count++;
    }
    
    return count;
    
}

void print_process_list(void) {
    int counter = 0;
    
    struct process_info *i;
    debug_printf("-----------------------------\n");
    debug_printf("Currently running processes:\n");
    debug_printf("PID\tCore\tName\n");
    debug_printf("%3d\t%4s\t%s\n", 0, "*", "init");
    for (i = process_list; i != NULL; i = i->next) {
        debug_printf("%3d\t%4d\t%s\n", i->pid, i->core_id, i->name);
        counter++;
    }
    debug_printf("Total number of processes: %d\n", counter);
    debug_printf("-----------------------------\n");
}
