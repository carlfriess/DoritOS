/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef DISTOPS_INTERNAL_H
#define DISTOPS_INTERNAL_H

#include <errors/errno.h>
#include <aos/debug.h>

#define PANIC_IF_ERR(err, msg) do { \
    errval_t tmp_err__ = (err); \
    if (err_is_fail(tmp_err__)) { \
        USER_PANIC_ERR(tmp_err__, (msg)); \
    } \
} while (0)

#endif
