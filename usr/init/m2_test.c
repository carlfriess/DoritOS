//
//  m2_test.c
//  DoritOS
//
//  Created by Sebastian Winberg on 14.10.17.
//

#include "m2_test.h"

#define PRINT_TEST_NAME         printf("\033[37m\033[40m%s\033[49m\033[39m\n", __FUNCTION__)
#define RETURN_TEST_SUCCESS        do { printf("\033[37m\033[42mSUCCESS\033[49m\033[39m\n"); return SYS_ERR_OK; } while(0)

void arun_all_m2_tests(void);


static size_t free_vspace(struct paging_state *st) {
    
    size_t free_space = 0;
    struct vspace_node *node = st->free_vspace_head;
    while (node != NULL) {
        free_space += node->size;
        node = node->next;
    }
    return free_space;
    
}

static errval_t test_map_unmap(size_t b) {
    PRINT_TEST_NAME;
    
    //struct paging_region *pr = NULL;
    //paging_region_init(get_current_paging_state(), pr, b);
    //size_t ret_size;
    //void *buff = NULL;
    //paging_region_map(pr, b, &buf, &ret_size);
    //paging_region_unmap(get_current_paging_state(), const void *region)
    
    char *buf;
    struct capref frame;
    size_t ret_bytes;
    
    errval_t err = frame_alloc(&frame, b, &ret_bytes);
    //debug_printf("%s\n", err_getstring(err));
    assert(err_is_ok(err));
    err = paging_map_frame(get_current_paging_state(), (void **) &buf, ret_bytes, frame, NULL, NULL);
    //debug_printf("%s\n", err_getstring(err));
    assert(err_is_ok(err));
    *buf = '*';    // ASCII 42
    //debug_printf("%d", i);
    
    //debug_printf("%c", *buf_array[i]);
    
    err = paging_unmap(get_current_paging_state(), (void *)buf);
    //debug_printf("%s\n", err_getstring(err));
    assert(err_is_ok(err));
    
    //debug_printf("%c", *buf);
    
    RETURN_TEST_SUCCESS;
}

static errval_t test_map_unmap_n(size_t b, size_t n) {
    PRINT_TEST_NAME;
    
    char *buf_array[n];
    for (int i = 0; i < n; ++i) {
        
        struct capref frame;
        size_t ret_bytes;
        errval_t err = frame_alloc(&frame, b, &ret_bytes);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        err = paging_map_frame(get_current_paging_state(), (void **)&(buf_array[i]), ret_bytes, frame, NULL, NULL);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        *buf_array[i] = '*';    // ASCII 42
        //debug_printf("%d", i);
        
    }
    
    for (int i = 0; i < n; ++i) {
        
        //debug_printf("%c", *buf_array[i]);
        
        errval_t err = paging_unmap(get_current_paging_state(), (void *)buf_array[i]);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        
        //debug_printf("%c", *buf_array[i]);
        
    }
    
    //debug_printf("%z/n", free_vspace(get_current_paging_state()) -  b*n);
    assert(free_vspace(get_current_paging_state()) - b*n <= 2*b);
    
    RETURN_TEST_SUCCESS;
}

static errval_t test_map_unmap_random(size_t b) {
    
    int rand[1000] = {11,54,46,48,13,65,25,39,88,43,18,27,81,84,40,2,30,6,50,63,23,33,29,87,4,70,96,74,44,32,86,90,89,19,79,94,95,97,51,56,100,64,21,10,62,35,72,12,24,68,26,20,67,3,9,14,92,77,61,73,93,5,15,55,98,17,71,83,78,36,45,8,47,1,75,60,99,69,42,37,16,34,58,28,22,66,80,91,41,7,52,59,76,38,57,49,31,85,82,53};
    
    char *buf_array[100];
    for (int i = 0; i < 100; ++i) {
        
        struct capref frame;
        size_t ret_bytes;
        errval_t err = frame_alloc(&frame, b, &ret_bytes);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        err = paging_map_frame(get_current_paging_state(), (void **)&(buf_array[i]), ret_bytes, frame, NULL, NULL);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        *buf_array[i] = '*';    // ASCII 42
        //debug_printf("%d", i);
        
    }
    
    for (int i = 0; i < 100; ++i) {
        
        //debug_printf("%c", *buf_array[i]);
        
        errval_t err = paging_unmap(get_current_paging_state(), (void *)buf_array[rand[i]-1]);
        debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        
        //debug_printf("%c", *buf_array[i]);
        
    }

    return 0;
}

static errval_t test_spawn_n(size_t n) {
    PRINT_TEST_NAME;

    for (int i = 0; i < n; i++) {
        debug_printf("===== %d =====\n", i+1);
        struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));
        spawn_load_by_name("hello", si);
        free(si);
    }
    
    RETURN_TEST_SUCCESS;
    
}

void arun_all_m2_tests(void) {
    
    printf("Test Phase 1: Simple map/unmap\n");
    test_map_unmap(BASE_PAGE_SIZE);
    
    printf("Test Phase 2: Repeat map/unmap\n");
    test_map_unmap_n(BASE_PAGE_SIZE, 200);
    
    printf("Test Phase 3: Large simple map/unmap\n");
    test_map_unmap(BASE_PAGE_SIZE * 512);
    
    printf("Test Phase 4: Large repeat map/unmap\n");
    test_map_unmap_n(BASE_PAGE_SIZE * 300, 500);
    
    printf("Test Phase 5: Random map/unmap\n");
    test_map_unmap_random(BASE_PAGE_SIZE);
    
    printf("Test Phase 6: Spawn children\n");
    test_spawn_n(10);
    
}

void run_all_m2_tests(void) {
    
    printf("Test Phase 1: Simple map/unmap\n");
    //test_map_unmap(BASE_PAGE_SIZE);
    
    printf("Test Phase 2: Repeat map/unmap\n");
    //test_map_unmap_n(BASE_PAGE_SIZE, 200);
    
    printf("Test Phase 3: Large simple map/unmap\n");
    //test_map_unmap(BASE_PAGE_SIZE * 512);
    
    printf("Test Phase 4: Large repeat map/unmap\n");
    //test_map_unmap_n(BASE_PAGE_SIZE * 300, 500);
    
    printf("Test Phase 5: Random map/unmap\n");
    test_map_unmap_random(BASE_PAGE_SIZE);
    
    printf("Test Phase 6: Spawn children\n");
    test_spawn_n(10);
    
}

