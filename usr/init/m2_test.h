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
#include <spawn/spawn.h>

#include <mm/mm.h>
#include "mem_alloc.h"

void run_all_m2_tests(void);

#endif /* m2_test_h */
