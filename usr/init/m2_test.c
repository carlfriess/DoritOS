//
//  m2_test.c
//  DoritOS
//
//  Created by Sebastian Winberg on 14.10.17.
//

#include "m2_test.h"

#define PRINT_TEST_NAME         printf("\033[37m\033[40m%s\033[49m\033[39m\n", __FUNCTION__)
#define RETURN_TEST_SUCCESS        do { printf("\033[37m\033[42mSUCCESS\033[49m\033[39m\n"); return SYS_ERR_OK; } while(0)

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
    PRINT_TEST_NAME;

    int rand[1000] = {494,251,920,75,518,467,159,1,606,698,477,635,272,69,397,377,852,692,982,850,37,395,296,826,170,108,551,179,531,679,204,634,53,446,315,805,103,722,100,544,714,757,970,755,758,505,581,851,949,952,403,693,918,997,766,77,302,843,611,300,209,674,68,592,730,436,460,402,440,431,687,370,688,503,514,889,550,640,865,971,680,963,483,866,149,713,916,245,161,226,434,444,808,780,162,931,276,500,369,999,836,347,965,830,886,961,260,95,967,775,175,670,994,959,132,392,424,771,148,871,379,216,584,847,254,703,232,825,12,996,975,637,612,709,269,594,789,879,591,854,708,998,470,799,685,493,973,972,893,178,717,921,653,476,807,856,950,842,648,880,740,152,319,593,117,205,642,337,310,373,15,182,769,508,699,280,32,614,619,430,864,221,354,603,285,587,252,651,641,400,580,981,94,731,711,910,772,20,549,288,303,198,119,697,91,583,267,915,665,47,748,831,576,672,798,295,404,657,219,127,427,955,329,560,948,632,784,496,867,261,707,104,788,902,207,533,778,27,927,956,684,13,481,817,988,248,761,137,937,331,912,774,448,990,715,335,547,607,621,16,488,357,995,70,537,41,243,38,558,196,328,235,938,134,391,906,625,613,73,43,138,399,87,358,922,39,837,59,660,438,82,733,790,794,462,859,14,797,875,942,813,696,652,384,724,721,502,143,111,79,723,72,186,406,645,543,506,958,795,265,782,858,944,752,229,381,36,694,598,834,980,66,478,62,345,473,199,268,238,555,811,214,114,129,855,767,80,378,153,46,522,223,90,482,454,40,597,993,365,99,608,215,150,270,861,405,976,213,42,745,702,445,928,590,1000,992,433,939,677,911,669,749,273,244,63,527,964,792,31,681,809,848,907,282,532,475,765,308,957,294,523,683,638,450,616,352,361,919,872,325,646,314,734,212,947,978,726,398,225,33,185,284,554,796,356,461,552,355,180,465,346,800,945,78,833,263,538,255,736,643,24,573,408,738,618,202,519,210,54,291,946,768,309,115,620,250,507,451,81,142,932,230,732,624,278,574,220,528,367,904,264,650,908,829,663,903,8,145,286,140,781,368,7,791,173,240,324,388,390,521,926,974,735,890,509,659,810,139,941,44,419,566,121,184,900,154,389,497,342,628,327,940,375,668,124,322,806,338,64,874,193,449,57,571,673,287,412,595,130,804,878,487,233,457,898,840,977,29,914,517,50,885,21,441,234,802,394,98,382,654,218,814,2,349,236,339,599,67,803,413,615,200,177,498,676,317,443,459,627,729,452,786,662,407,857,535,983,332,163,718,231,30,666,195,28,690,239,136,380,553,206,815,630,515,58,429,565,71,11,862,76,631,899,471,453,747,513,725,418,442,629,924,756,743,933,48,968,559,359,838,863,168,435,362,158,821,171,106,841,529,49,819,542,604,605,876,96,577,504,561,827,151,387,344,744,386,56,374,410,545,92,822,83,884,754,909,118,720,192,166,203,274,88,689,4,26,316,3,458,60,485,266,602,249,126,966,45,582,636,172,275,556,706,155,492,372,242,188,883,905,647,259,801,564,85,432,51,102,846,712,728,19,548,376,586,536,891,466,469,727,472,409,655,649,678,17,484,107,929,960,579,463,464,913,383,348,539,23,128,241,201,89,169,144,901,417,793,468,596,277,658,985,74,414,113,705,777,600,423,439,351,499,530,639,667,426,563,304,360,364,396,131,984,174,341,246,623,853,292,609,644,589,671,237,311,156,307,247,22,330,845,987,135,776,416,191,569,157,52,109,661,289,779,183,176,524,562,783,480,568,456,257,320,290,93,256,546,512,823,936,190,224,65,818,197,271,710,86,5,923,664,868,824,887,18,787,421,306,764,490,334,588,760,695,869,567,762,340,622,323,785,860,770,105,578,366,820,575,935,585,298,9,181,835,526,969,165,326,164,962,350,888,227,682,572,525,954,61,894,849,336,741,455,617,84,510,753,305,318,133,489,187,495,828,217,208,742,420,146,832,686,301,299,691,112,10,141,701,122,333,411,393,943,704,979,716,882,541,167,897,989,447,34,25,253,321,719,570,895,125,123,228,479,739,279,353,656,194,262,6,675,97,35,934,763,474,422,516,283,986,401,540,428,930,750,116,881,925,626,812,991,293,610,759,534,437,511,773,491,385,633,258,844,211,110,222,101,751,873,425,363,147,737,312,746,415,281,297,557,601,896,877,120,917,870,700,892,520,189,951,953,816,55,160,501,486,839,343,313,371};
    
    char *buf_array[1000];
    for (int i = 0; i < 1000; ++i) {
        
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
    
    for (int i = 0; i < 1000; ++i) {
        
        //debug_printf("%c", *buf_array[i]);
        
        errval_t err = paging_unmap(get_current_paging_state(), (void *)buf_array[rand[i]-1]);
        //debug_printf("%s\n", err_getstring(err));
        assert(err_is_ok(err));
        
        //debug_printf("%c", *buf_array[i]);
        
    }
    
    RETURN_TEST_SUCCESS;
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

void run_all_m2_tests(void) {
    
    printf("Test Phase 1: Simple map/unmap\n");
    if (false)
        test_map_unmap(BASE_PAGE_SIZE);
    
    printf("Test Phase 2: Repeat map/unmap\n");
    if (false)
        test_map_unmap_n(BASE_PAGE_SIZE, 200);
    
    printf("Test Phase 3: Large simple map/unmap\n");
    if (false)
        test_map_unmap(BASE_PAGE_SIZE * 512);
    
    printf("Test Phase 4: Large repeat map/unmap\n");
    if (false)
        test_map_unmap_n(BASE_PAGE_SIZE * 300, 500);
    
    printf("Test Phase 5: Random map/unmap\n");
    if (false)
        test_map_unmap_random(BASE_PAGE_SIZE*3);
    
    printf("Test Phase 6: Spawn children\n");
    if (true)
        test_spawn_n(10);
    
}

