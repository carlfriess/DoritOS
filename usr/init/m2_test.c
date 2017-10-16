//
//  m2_test.c
//  DoritOS
//
//  Created by Sebastian Winberg on 14.10.17.
//

#include "m2_test.h"

errval_t test_paging_alloc_n(size_t b) {
    PRINT_TEST_NAME;

    //paging_alloc(get_current_paging_state(), &buf, b);
    
    RETURN_TEST_SUCCESS;
}

errval_t test_map_unmap_n(size_t b, int n) {
    PRINT_TEST_NAME;

    //struct paging_region *pr = NULL;
    //paging_region_init(get_current_paging_state(), pr, b);
    //size_t ret_size;
    //void *buff = NULL;
    //paging_region_map(pr, b, &buf, &ret_size);
    //paging_region_unmap(get_current_paging_state(), const void *region)
    
    char *buf_array[n];
    for (int i = 0; i < n; ++i) {
        
        struct capref frame;
        size_t ret_bytes;
        frame_alloc(&frame, b, &ret_bytes);
        debug_printf("---------->ret_bytes: %d\n", ret_bytes);

        paging_map_frame(get_current_paging_state(), (void **)&(buf_array[i]), ret_bytes, frame, NULL, NULL);
        
        debug_printf("---------->buf_array[i]: %x\n", buf_array[i]);
        
        *buf_array[i] = '*';
        
        debug_printf("---------->*buf_array[i]: %d\n", *buf_array[i]);

        
    }
    
    for (int i = 0; i < n; ++i) {
        paging_unmap(get_current_paging_state(), (void *)buf_array[i]);
        
        debug_printf("---------->*buf_array[i]: %d\n", *buf_array[i]);

    }
        
    RETURN_TEST_SUCCESS;
}

void run_all_m2_tests(void) {
    
    printf("Test Phase 1: \n");
    test_map_unmap_n(4096, 4);

}

