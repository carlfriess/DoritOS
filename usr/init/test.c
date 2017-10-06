//
//  test.c
//  DoritOS
//
//  Created by Sebastian Winberg on 06.10.17.
//

#include "test.h"


/// a1 f1 with (b)
errval_t test_alloc_free(size_t b) {
    PRINT_TEST_NAME;

    struct capref retcap;
    
    assert(err_is_ok( ram_alloc(&retcap, b) ));
    assert(err_is_ok( aos_ram_free(retcap, b) ));
    
    RETURN_TEST_SUCCESS;
}

/// a1 f1 a2 f2 with (b1,b2)
errval_t test_alloc_free_alloc_free(size_t b1, size_t b2) {
    PRINT_TEST_NAME;

    struct capref retcap1;
    struct capref retcap2;
    
    assert(err_is_ok( ram_alloc(&retcap1, b1) ));
    assert(err_is_ok( aos_ram_free(retcap1, b1) ));
    assert(err_is_ok( ram_alloc(&retcap2, b2) ));
    assert(err_is_ok( aos_ram_free(retcap2, b2) ));
    
    RETURN_TEST_SUCCESS;
}

/// a1 f1 f1 with (b)
errval_t test_alloc_free_free(size_t b) {
    PRINT_TEST_NAME;
    
    struct capref retcap;
    
    assert(err_is_ok( ram_alloc(&retcap, b) ));
    assert(err_is_ok( aos_ram_free(retcap, b) ));
    assert(!err_is_ok( aos_ram_free(retcap, b) ));
    
    RETURN_TEST_SUCCESS;
}

/// a1..an and f1..fn with (b)
errval_t test_alloc_n_free_n(int n, size_t b) {
    PRINT_TEST_NAME;
    
    struct capref retcap_array[n];
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array[i]), b) ));
    }
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array[i], b) ));
    }
    
    RETURN_TEST_SUCCESS;
}

/// a1 a2 a3 f3 f1 f2 with (b1,b2,b3)
errval_t test_coalescing1(size_t b1, size_t b2, size_t b3) {
    PRINT_TEST_NAME;

    struct capref retcap_1;
    struct capref retcap_2;
    struct capref retcap_3;
    
    assert(err_is_ok( ram_alloc(&retcap_1, b1) ));
    assert(err_is_ok( ram_alloc(&retcap_2, b2) ));
    assert(err_is_ok( ram_alloc(&retcap_3, b3) ));
    
    assert(err_is_ok( aos_ram_free(retcap_3, b3) ));
    assert(err_is_ok( aos_ram_free(retcap_1, b1) ));
    assert(err_is_ok( aos_ram_free(retcap_2, b2) ));
    
    RETURN_TEST_SUCCESS;
}

errval_t test_slot_alloc_n(int n) {
    PRINT_TEST_NAME;

    struct capref retcap_array[n];
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( slot_alloc(&retcap_array[i]) ));
    }
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( slot_free(retcap_array[i]) ));
    }
    
    RETURN_TEST_SUCCESS;
}

errval_t test_frame_alloc(size_t b) {
    PRINT_TEST_NAME;

    struct capref frame;
    size_t retSize;
    assert(err_is_ok( frame_alloc(&frame, b, &retSize) ));
    //debug_printf("Allocated a %zu byte frame: %s\n", retSize, err_getstring(err1));
    
    RETURN_TEST_SUCCESS;
}

errval_t test_frame_alloc_n(int n, size_t b) {
    PRINT_TEST_NAME;
    
    for (int i=0; i<n; ++i) {
        struct capref frame;
        size_t retSize;
        assert(err_is_ok( frame_alloc(&frame, b, &retSize) ));
    }
    
    RETURN_TEST_SUCCESS;
}

errval_t test_ram_leak(int n, size_t b) {
    PRINT_TEST_NAME;

    gensize_t available[4] = { 0, 0, 0, 0 };
    gensize_t total[4];
    
    struct capref retcap_array1[n];
    //struct capref retcap_array2[n];
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array1[i]), b) ));
    }
    
    mm_available(&aos_mm, &available[0], &total[0]);

    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array1[i], b) ));
    }
    
    mm_available(&aos_mm, &available[1], &total[1]);
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array1[i]), b) ));
    }
    
    mm_available(&aos_mm, &available[2], &total[2]);

    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array1[i], b) ));
    }
    
    mm_available(&aos_mm, &available[3], &total[3]);

    debug_printf("available[]: %llu %llu %llu %llu\n", available[0], available[1], available[2], available[3]);
    
    debug_printf("total[]: %llu %llu %llu %llu\n", total[0], total[1], total[2], total[3]);
    
    assert(available[0] == available[2] &&
           available[1] == available[3]);
    assert(total[0] == total[1] &&
           total[1] == total[2] &&
           total[2] == total[3]);
    
    
    RETURN_TEST_SUCCESS;
}

void run_all_tests(void) {
    
    /* TESTS */
    /*
    printf("Test Phase 1\n");
    test_alloc_free_alloc_free(64, 64);
    
    printf("Test Phase 2\n");
    test_alloc_free(4096);
    //test_alloc_free(0);      //< Should this be fixed?
    
    printf("Test Phase 3\n");
    test_alloc_free_free(2*4096);
    
    printf("Test Phase 4\n");
    test_alloc_n_free_n(64, 2048);
    test_alloc_n_free_n(200, 4096);
    */
     test_alloc_n_free_n(500, 3*4096);         //< Not working
    /*
    printf("Test Phase 5\n");
    test_coalescing1(2048, 4096, 2*4096);
    test_coalescing1(2*4096, 4096, 1);
    
    //printf("Test 6\n");
    // TODO: Check other coalescing cases
    
    printf("Test Phase 6\n");
    //test_ram_leak(1000, 4096);
    */
    
    printf("Test Phase 7\n");
    //test_frame_alloc(4096);
    //test_frame_alloc(8*4096);
    //test_frame_alloc_n(100, 4096);
    
}
