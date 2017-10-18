//
//  m2_test.h
//  DoritOS
//
//  Created by Sebastian Winberg on 14.10.17.
//

#ifndef m2_test_h
#define m2_test_h

#include <stdio.h>
#include <stdlib.h>

#include <aos/aos.h>
#include <aos/waitset.h>
#include <aos/morecore.h>
#include <aos/paging.h>

#include <mm/mm.h>
#include "mem_alloc.h"

errval_t test_paging_alloc_n(size_t b);

errval_t test_map_unmap_n(size_t b, int n);

errval_t test_map_unmap_random(void);

void run_all_m2_tests(void);

#define PRINT_TEST_NAME         printf("Test %s: ", __FUNCTION__)
#define RETURN_TEST_SUCCESS        do { printf("\033[37m\033[42mSUCCESS\033[49m\033[39m\n"); return SYS_ERR_OK; } while(0)

#endif /* m2_test_h */
