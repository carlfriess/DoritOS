/**
 * \file
 * \brief Hello world application
 */

/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, CAB F.78, Universitaetstr. 6, CH-8092 Zurich,
 * Attn: Systems Group.
 */


#include <stdio.h>

#include <aos/aos.h>
#include <fs/fs.h>
#include <fs/dirent.h>
#include <omap_timer/timer.h>

#define ENABLE_LONG_FILENAME_TEST 1

/* reading */
#define MOUNTPOINT     "/sdcard"
#define SUBDIR         "/parent"
#define SUBDIR_LONG    "/parent-directory"
#define DIR_NOT_EXIST  "/not-exist"
#define FILENAME       "/myfile.txt"
#define LONGFILENAME   "/mylongfilenamefile.txt"
#define LONGFILENAME2  "/mylongfilenamefilesecond.txt"
#define FILE_NOT_EXIST "/not-exist.txt"

#define TEST_PREAMBLE(arg) \
    printf("\n-------------------------------\n"); \
    printf("%s(%s)\n", __FUNCTION__, arg);

#define TEST_END \
    printf("-------------------------------\n");

#define EXPECT_SUCCESS(err, test, _time) \
    do{ \
        if(err_is_fail(err)){ \
            DEBUG_ERR(err, test); \
        } else { \
            printf("SUCCESS: " test " took %" PRIu64 "\n", _time);\
        }\
   } while(0);\

#define EXPECT_FAILURE(err, _test, _time) \
    do{ \
        if(err_is_fail(err)){ \
            printf("SUCCESS: failure expected " _test " took %" PRIu64 " ms\n", _time);\
        } else { \
            DEBUG_ERR(err, "FAILURE: failure expected, but test succeeded" _test); \
        }\
   } while(0);\

#define run_test(fn, arg) \
    do { \
        tstart = omap_timer_read(); \
        err = fn(arg); \
        tend = omap_timer_read(); \
        EXPECT_SUCCESS(err, #fn, omap_timer_to_ms(tend-tstart)); \
        TEST_END \
    } while(0); \

#define run_test_fail(fn, arg) \
    do { \
        tstart = omap_timer_read(); \
        err = fn(arg); \
        tend = omap_timer_read(); \
        EXPECT_FAILURE(err, #fn, omap_timer_to_ms(tend-tstart)); \
        TEST_END \
    } while(0); \




static errval_t test_read_dir(char *dir)
{
    errval_t err;

    TEST_PREAMBLE(dir)

    fs_dirhandle_t dh;
    err = opendir(dir, &dh);
    if (err_is_fail(err)) {
        return err;
    }

    assert(dh);

    do {
        char *name;
        err = readdir(dh, &name);
        if (err_no(err) == FS_ERR_INDEX_BOUNDS) {
            break;
        } else if (err_is_fail(err)) {
            goto err_out;
        }
        printf("%s\n", name);
    } while(err_is_ok(err));

    return closedir(dh);
    err_out:
    return err;
}

static errval_t test_fread(char *file)
{
    int res = 0;

    TEST_PREAMBLE(file)

    FILE *f = fopen(file, "r");
    if (f == NULL) {
        return FS_ERR_OPEN;
    }

    /* obtain the file size */
    res = fseek (f , 0 , SEEK_END);
    if (res) {
        return FS_ERR_INVALID_FH;
    }

    size_t filesize = ftell (f);
    rewind (f);

    printf("File size is %zu\n", filesize);

    char *buf = calloc(filesize + 2, sizeof(char));
    if (buf == NULL) {
        return LIB_ERR_MALLOC_FAIL;
    }

    size_t read = fread(buf, 1, filesize, f);

    printf("read: %s\n", buf);

    if (read != filesize) {
        return FS_ERR_READ;
    }

    rewind(f);

    size_t nchars = 0;
    int c;
    do {
        c = fgetc (f);
        nchars++;
    } while (c != EOF);

    if (nchars < filesize) {
        return FS_ERR_READ;
    }

    free(buf);
    res = fclose(f);
    if (res) {
        return FS_ERR_CLOSE;
    }

    return SYS_ERR_OK;
}


int main(int argc, char *argv[])
{
    errval_t err;
    uint64_t tstart, tend;

    printf("Filereader test\n");

    err = omap_timer_init();
    assert(err_is_ok(err));

    printf("init timer\n");
    omap_timer_ctrl(true);

    printf("initializing filesystem...\n");
    err = filesystem_init();
    EXPECT_SUCCESS(err, "failure during fs init", 0);

    err = filesystem_mount("/sdcard", "mmchs://fat32/0");
    EXPECT_SUCCESS(err, "failure during fs mount", 0);

    run_test(test_read_dir, MOUNTPOINT "/");

    run_test_fail(test_read_dir, DIR_NOT_EXIST);

    run_test(test_fread, MOUNTPOINT FILENAME);

    return EXIT_SUCCESS;
}
