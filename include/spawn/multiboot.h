
#ifndef MULTIBOOT_H_
#define MULTIBOOT_H_

#include <aos/aos.h>

// Return module line starting at the binary name
const char * multiboot_module_opts(struct mem_region * module);

// Return module line as written in menu.lst
const char * multiboot_module_rawstring(struct mem_region *region);

// Find mem_region for a given module name
struct mem_region *multiboot_find_module(struct bootinfo *bi, const char *name);

// Return module name
const char *multiboot_module_name(struct mem_region *region);

#endif
