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
        paging_map_frame(get_current_paging_state(), (void **)&(buf_array[i]), ret_bytes, frame, NULL, NULL);
        *buf_array[i] = '*';    // ASCII 42
        
    }
    
    for (int i = 0; i < n; ++i) {
        
        paging_unmap(get_current_paging_state(), (void *)buf_array[i]);
    
    }
    
    /*
     for (int i = 0; i < 200; i++) {
     if (seq[i] > 0) {
     errval_t err = ram_alloc(&(retcap[seq[i]-1]), seq[i]*100);
     assert(err_is_ok(err));
     } else {
     errval_t err = aos_ram_free(retcap[(-seq[i])-1], (-seq[i])*100);
     assert(err_is_ok(err));
     }
     }
     */
    
        
    RETURN_TEST_SUCCESS;
}

errval_t test_map_unmap_random(void) {
    return 0;
}

void run_all_m2_tests(void) {
    
    printf("Test Phase 1: \n");
    test_map_unmap_n(4096, 4);

}

