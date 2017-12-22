//
//  fat_helper.c
//  DoritOS
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <aos/aos.h>

#include <fs_serv/fat_helper.h>

#define PRINT_DEBUG 0

char *convert_to_fat_name(const char *name) {
    
    // Length of name (not including '\0')
    size_t len = strlen(name);
    
    // Allocate return name string with length 12 (one extra for '\0')
    char *ret_name = calloc(1, 12);
    
    // Set memory to space char
    memset(ret_name, ' ', 12);
    
    // Set null terminator at the end
    ret_name[11] = '\0';
    
    // Head pointer of return name for max 8 bit big prefix
    char *head = ret_name;
    
    // Tail pointer of return name for max 3 bit big extension
    char *tail = ret_name + 8;
    
    // Parse input and split between dot
    char *dot_ptr = strrchr(name, FS_EXTENSION_SEP);
    
    // Size of prefix part before implicit dot
    size_t prefix_size;
    
    // Size of extension part after implicit dot
    size_t extension_size;
    
    if (dot_ptr == NULL) {
#if PRINT_DEBUG
        debug_printf("No dot found in filename\n");
#endif
        // Set prefix and extension size
        prefix_size = len;
        extension_size = 0;
        
    } else {
        
        // Set prefix and extension size
        prefix_size = dot_ptr - name;
        extension_size = len - (dot_ptr + 1 - name);
        
        // Increment dot pointer to point to first extension letter
        dot_ptr += 1;
        
    }
    
    // Check if extension size is small enough and otherwise shorten it to 3
    if (extension_size > 3) {
#if PRINT_DEBUG
        debug_printf("Extension of filename is too long\n");
#endif
        extension_size = 3;
    }
    
    // Copy UPPERCASE extension letters into result name tail part
    for (int i = 0; i < extension_size; i++) {
        tail[i] = toupper(dot_ptr[i]);
    }
    
    // Check if prefix_size size is small enough and otherwise shorten it to 8
    if (prefix_size > 8) {
#if PRINT_DEBUG
        debug_printf("Prefix of filename is too long\n");
#endif
        prefix_size = 8;
    }
    
    // Copy UPPERCASE prefic letters into result name head part
    for (int i = 0; i < prefix_size; i++) {
        head[i] = toupper(name[i]);
    }
    
    return ret_name;
    
}

char *convert_to_normal_name(char *fat_name) {
    
    // Head pointer of fat name
    char *head = fat_name;
    
    // Tail pointer of fat name
    char *tail = fat_name + 8;
    
    // Calculate size of extension part (disregarding spaces at the end)
    size_t prefix_size = 0;
    for (int i = 0; i < 8; i++) {
        if (head[i] != ' ') {
            prefix_size = i + 1;
        }
    }
    
    // Calculate size of extension part (disregarding spaces at the end)
    size_t extension_size = 0;
    for (int i = 0; i < 3; i++) {
        if (tail[i] != ' ') {
            extension_size = i + 1;
        }
    }
    
    // Initialize return name
    char *ret_name;
    
    // Check if we should add a dot to return name
    if (extension_size != 0) {
        
        // Allocate return name (with '.' and '\0')
        ret_name = calloc(1, prefix_size + extension_size + 2);
        
        // Copy prefix string into return name
        memcpy(ret_name, head, prefix_size);
        
        // Set dot in return name
        ret_name[prefix_size] = '.';
        
        // Copy extension string into return name
        memcpy(ret_name + prefix_size + 1, tail, extension_size);
        
        // Set null terminator in return name
        ret_name[prefix_size + extension_size + 1] = '\0';
        
    } else {
        
        // Allocate return name (with '\0')
        ret_name = calloc(1, prefix_size + extension_size + 1);
        
        // Copy prefix string into return name
        memcpy(ret_name, head, prefix_size);
        
        // Set null terminator in return name
        ret_name[prefix_size + extension_size] = '\0';
        
    }
    
    return ret_name;
    
}
