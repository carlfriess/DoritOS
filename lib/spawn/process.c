#include <aos/aos.h>
#include <spawn/process.h>

//static size_t pid = 0;
struct process_info *process_list = NULL;


void process_register(struct process_info *pi) {
    struct process_info **node;
    for (node = &process_list; *node != NULL; node = &((*node)->next));
    *node = pi;
}

void print_process_list(void) {
    size_t counter = 1;

    struct process_info *i;
    debug_printf("Currently running processes:\n");
    debug_printf("\t%3d\t%s\n", 0, "init");
    for (i = process_list; i != NULL; i = i->next) {
        debug_printf("\t%3d\t%s\n", i->pid, i->name);
        counter++;
    }
    debug_printf("Total number of processes: %d\n", counter);
}