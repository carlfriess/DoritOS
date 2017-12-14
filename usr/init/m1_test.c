//
//  m1_test.c
//  DoritOS
//
//  Created by Sebastian Winberg on 06.10.17.
//

#include "m1_test.h"

errval_t test_alloc_free(size_t b) {
    PRINT_TEST_NAME;

    struct capref retcap;
    
    assert(err_is_ok( ram_alloc(&retcap, b) ));
    assert(err_is_ok( aos_ram_free(retcap) ));
    
    RETURN_TEST_SUCCESS;
}

errval_t test_alloc_free_alloc_free(size_t b1, size_t b2) {
    PRINT_TEST_NAME;

    struct capref retcap1;
    struct capref retcap2;
    
    assert(err_is_ok( ram_alloc(&retcap1, b1) ));
    assert(err_is_ok( aos_ram_free(retcap1) ));
    assert(err_is_ok( ram_alloc(&retcap2, b2) ));
    assert(err_is_ok( aos_ram_free(retcap2) ));
    
    RETURN_TEST_SUCCESS;
}

errval_t test_alloc_free_free(size_t b) {
    PRINT_TEST_NAME;
    
    struct capref retcap;
    
    assert(err_is_ok( ram_alloc(&retcap, b) ));
    assert(err_is_ok( aos_ram_free(retcap) ));
    assert(aos_ram_free(retcap) == MM_ERR_NOT_FOUND);
    
    RETURN_TEST_SUCCESS;
}

errval_t test_alloc_n_free_n(int n, size_t b) {
    PRINT_TEST_NAME;
    
    struct capref retcap_array[n];
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array[i]), b) ));
    }
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array[i]) ));
    }
    
    RETURN_TEST_SUCCESS;
}

errval_t test_coalescing(size_t b1, size_t b2, size_t b3) {
    PRINT_TEST_NAME;

    struct capref retcap_1;
    struct capref retcap_2;
    struct capref retcap_3;
    
    assert(err_is_ok( ram_alloc(&retcap_1, b1) ));
    assert(err_is_ok( ram_alloc(&retcap_2, b2) ));
    assert(err_is_ok( ram_alloc(&retcap_3, b3) ));
    
    assert(err_is_ok( aos_ram_free(retcap_3) ));
    assert(err_is_ok( aos_ram_free(retcap_1) ));
    assert(err_is_ok( aos_ram_free(retcap_2) ));
    
    RETURN_TEST_SUCCESS;
}

errval_t test_ram_leak(int n, size_t b) {
    PRINT_TEST_NAME;

    gensize_t available[4] = { 0, 0, 0, 0 };
    gensize_t total[4] = { 0, 0, 0, 0 };
    
    struct capref retcap_array1[n];
    struct capref retcap_array2[n];
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array1[i]), b) ));
    }
    
    mm_available(&aos_mm, &available[0], &total[0]);

    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array1[i]) ));
    }
    
    mm_available(&aos_mm, &available[1], &total[1]);
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array2[i]), b) ));
    }
    
    mm_available(&aos_mm, &available[2], &total[2]);

    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array2[i]) ));
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

errval_t test_inc_n_by_k(int n, int k) {
    PRINT_TEST_NAME;

    gensize_t available[4] = { 0, 0, 0, 0 };
    gensize_t total[4] = { 0, 0, 0, 0 };
    
    struct capref retcap_array1[n];
    struct capref retcap_array2[n];
    
    for (int i=n-1; 0<=i; --i) {
        assert(err_is_ok( ram_alloc(&(retcap_array1[i]), k*(i+1)) ));
    }
    
    mm_available(&aos_mm, &available[0], &total[0]);
    
   
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( aos_ram_free(retcap_array1[i]) ));
    }
    
    mm_available(&aos_mm, &available[1], &total[1]);
    
    for (int i=0; i<n; ++i) {
        assert(err_is_ok( ram_alloc(&(retcap_array2[i]), k*(i+1)) ));
    }
    
    mm_available(&aos_mm, &available[2], &total[2]);
    
    for (int i=n-1; 0<=i; --i) {
        assert(err_is_ok( aos_ram_free(retcap_array2[i]) ));
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

errval_t test_random_seq(void) {
    PRINT_TEST_NAME;
    
    int seq[200] = {33,93,80,75,65,-33,71,-93,42,88,-42,-71,32,8,7,31,6,-32,40,13,-65,-6,17,-7,-13,49,-49,5,98,69,25,-75,4,34,37,48,15,60,87,100,1,81,-8,99,-48,39,58,41,-60,-88,91,24,-5,82,18,52,-69,74,67,-40,10,72,27,89,30,20,46,94,83,51,53,66,-82,86,-98,68,-51,3,14,79,45,2,64,54,85,-80,-66,61,28,-94,-53,55,70,84,76,-34,73,-54,-74,50,57,-27,19,-37,22,92,-3,78,59,96,95,43,62,26,-41,97,21,-73,38,44,47,35,-1,90,11,23,36,-55,63,-24,29,77,16,-95,56,9,12,-10,-4,-25,-97,-30,-56,-64,-79,-18,-16,-23,-46,-78,-35,-81,-17,-100,-85,-72,-99,-77,-92,-89,-28,-67,-84,-45,-21,-96,-14,-87,-22,-57,-15,-63,-29,-50,-76,-83,-9,-36,-47,-61,-31,-39,-12,-20,-59,-58,-2,-68,-26,-19,-43,-11,-86,-38,-70,-62,-91,-90,-44,-52};
    
    struct capref retcap[200];
    gensize_t available[2] = { 0, 0 };
    gensize_t total[2] = { 0, 0 };
    
    mm_available(&aos_mm, &available[0], &total[0]);
    
    for (int i = 0; i < 200; i++) {
        if (seq[i] > 0) {
            errval_t err = ram_alloc(&(retcap[seq[i]-1]), seq[i]*100);
            assert(err_is_ok(err));
        } else {
            errval_t err = aos_ram_free(retcap[(-seq[i])-1]);
            assert(err_is_ok(err));
        }
    }
    
    mm_available(&aos_mm, &available[1], &total[1]);
    
    debug_printf("available[]: %llu %llu\n", available[0], available[1]);
    
    debug_printf("total[]: %llu %llu\n", total[0], total[1]);
    
    assert(available[0] == available[1]);
    assert(total[0] == total[1]);
    
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


void run_all_m1_tests(void) {
    
    /* TESTS */
    
    printf("Test Phase 1: simple tests\n");
    test_alloc_free(64);
    test_alloc_free(4096);
    test_alloc_free_alloc_free(64, 128);
    
    printf("Test Phase 2: error tests\n");
    test_alloc_free_free(32);
    test_alloc_free_free(2*4096);
    
    printf("Test Phase 3: heavy load tests\n");
    test_alloc_n_free_n(64, 2048);
    test_alloc_n_free_n(200, 4096);
    test_alloc_n_free_n(500, 3*4096);
    
    printf("Test Phase 4: coalescing tests\n");
    test_coalescing(2048, 4096, 2*4096);
    test_coalescing(2*4096, 4096, 1);
    
    printf("Test Phase 5: ram leak tests\n");
    test_ram_leak(1000, 2096);
    
    printf("Test Phase 6: increment ram leak tests\n");
    test_inc_n_by_k(1000, 7);

    printf("Test Phase 7: random ram leak tests\n");
    test_random_seq();
    
    printf("Test Phase 8: frame allocation tests\n");
    test_frame_alloc(4096);
    test_frame_alloc(8*4096);
    test_frame_alloc_n(100, 8192);
    
}
