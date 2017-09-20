/**
 * \file
 * \brief Bidirectional LMP channel implementation
 */

/*
 * Copyright (c) 2009, 2010, 2011, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <aos/aos.h>
#include <aos/lmp_chan.h>
#include <aos/dispatcher_arch.h>
#include <aos/caddr.h>
#include <aos/waitset_chan.h>
#include "waitset_chan_priv.h"

/**
 * \brief Initialise a new LMP channel
 *
 * \param lc  Storage for channel state
 */
void lmp_chan_init(struct lmp_chan *lc)
{
    assert(lc != NULL);
    lc->connstate = LMP_DISCONNECTED;
    waitset_chanstate_init(&lc->send_waitset, CHANTYPE_LMP_OUT);
    lc->endpoint = NULL;
#ifndef NDEBUG
    lc->prev = lc->next = NULL;
#endif
}

/// Destroy the local state associated with a given channel
void lmp_chan_destroy(struct lmp_chan *lc)
{
    lc->connstate = LMP_DISCONNECTED;
    cap_destroy(lc->local_cap);

    if (lc->endpoint != NULL) {
        lmp_endpoint_free(lc->endpoint);
    }

    // remove from send retry queue on dispatcher
    if (waitset_chan_is_registered(&lc->send_waitset)) {
        assert(lc->prev != NULL && lc->next != NULL);
        dispatcher_handle_t handle = disp_disable();
        struct dispatcher_generic *disp = get_dispatcher_generic(handle);
        if (lc->next == lc->prev) {
            assert_disabled(lc->next == lc);
            assert_disabled(disp->lmp_send_events_list == lc);
            disp->lmp_send_events_list = NULL;
        } else {
            lc->prev->next = lc->next;
            lc->next->prev = lc->prev;
        }
        disp_enable(handle);

#ifndef NDEBUG
        lc->next = lc->prev = NULL;
#endif
    }

    waitset_chanstate_destroy(&lc->send_waitset);
}

/**
 * \brief Initialise a new LMP channel to accept an incoming binding request
 *
 * \param lc  Storage for channel state
 * \param buflen_words Size of incoming buffer, in words
 * \param endpoint Capability to remote LMP endpoint
 */
errval_t lmp_chan_accept(struct lmp_chan *lc,
                         size_t buflen_words, struct capref endpoint)
{
    errval_t err;

    lmp_chan_init(lc);
    lc->remote_cap = endpoint;

    /* allocate a cap slot for the new endpoint cap */
    err = slot_alloc(&lc->local_cap);
    if (err_is_fail(err)) {
        return err_push(err, LIB_ERR_SLOT_ALLOC);
    }

    /* allocate a local endpoint */
    err = lmp_endpoint_create_in_slot(buflen_words, lc->local_cap,
                                      &lc->endpoint);
    if (err_is_fail(err)) {
        slot_free(lc->local_cap);
        return err_push(err, LIB_ERR_ENDPOINT_CREATE);
    }

    /* mark connected */
    lc->connstate = LMP_CONNECTED;
    return SYS_ERR_OK;
}

/**
 * \brief Register an event handler to be notified when messages can be sent
 *
 * In the future, call the closure on the given waitset when it is likely that
 * a message can be sent on the channel. A channel may only be registered
 * with a single send event handler on a single waitset at any one time.
 *
 * \param lc LMP channel
 * \param ws Waitset
 * \param closure Event handler
 */
errval_t lmp_chan_register_send(struct lmp_chan *lc, struct waitset *ws,
                                 struct event_closure closure)
{
    assert(lc != NULL);
    assert(ws != NULL);

    errval_t err = waitset_chan_register(ws, &lc->send_waitset, closure);
    if (err_is_fail(err)) {
        return err;
    }

    // enqueue in list of channels with a registered event to retry sending
    assert(lc->next == NULL && lc->prev == NULL);
    dispatcher_handle_t handle = disp_disable();
    struct dispatcher_generic *dp = get_dispatcher_generic(handle);
    if (dp->lmp_send_events_list == NULL) {
        dp->lmp_send_events_list = lc;
        lc->next = lc->prev = lc;
    } else {
        lc->prev = dp->lmp_send_events_list->prev;
        lc->next = dp->lmp_send_events_list;
        lc->prev->next = lc;
        lc->next->prev = lc;
    }
    disp_enable(handle);

    return err;
}

/**
 * \brief Cancel an event registration made with lmp_chan_register_send()
 *
 * \param lc LMP channel
 */
errval_t lmp_chan_deregister_send(struct lmp_chan *lc)
{
    assert(lc != NULL);
    errval_t err = waitset_chan_deregister(&lc->send_waitset);
    if (err_is_fail(err)) {
        return err;
    }

    // dequeue from list of channels with send events
    assert(lc->next != NULL && lc->prev != NULL);
    dispatcher_handle_t handle = disp_disable();
    struct dispatcher_generic *dp = get_dispatcher_generic(handle);
    if (lc->next == lc->prev) {
        assert_disabled(dp->lmp_send_events_list == lc);
        dp->lmp_send_events_list = NULL;
    } else {
        lc->prev->next = lc->next;
        lc->next->prev = lc->prev;
        if (dp->lmp_send_events_list == lc) {
            dp->lmp_send_events_list = lc->next;
        }
    }
#ifndef NDEBUG
    lc->prev = lc->next = NULL;
#endif

    disp_enable(handle);
    return err;
}

/**
 * \brief Migrate an event registration to a new waitset.
 *
 * \param lc LMP channel
 * \param ws New waitset to migrate to
 */
void lmp_chan_migrate_send(struct lmp_chan *lc, struct waitset *ws)
{
    assert(lc != NULL);
    waitset_chan_migrate(&lc->send_waitset, ws);
}

/**
 * \brief Allocate a new receive capability slot for an LMP channel
 *
 * This utility function allocates a new receive slot (using #slot_alloc)
 * and sets it on the channel (using #lmp_chan_set_recv_slot).
 *
 * \param lc LMP channel
 */
errval_t lmp_chan_alloc_recv_slot(struct lmp_chan *lc)
{
    struct capref slot;

    errval_t err = slot_alloc(&slot);
    if (err_is_fail(err)) {
        return err_push(err, LIB_ERR_SLOT_ALLOC);
    }

    lmp_chan_set_recv_slot(lc, slot);
    return SYS_ERR_OK;
}

/**
 * \brief Trigger send events for all LMP channels that are registered
 *
 * We don't have a good way to determine when we are likely to be able
 * to send on an LMP channel, so this function just trigger all such
 * pending events every time the dispatcher is rescheduled.
 *
 * Must be called while disabled and from dispatcher logic.
 */
void lmp_channels_retry_send_disabled(dispatcher_handle_t handle)
{
    struct dispatcher_generic *dp = get_dispatcher_generic(handle);
    struct lmp_chan *lc, *first = dp->lmp_send_events_list, *next;
    errval_t err;

    for (lc = first; lc != NULL; lc = next) {
        next = lc->next;
        assert(next != NULL);
        err = waitset_chan_trigger_disabled(&lc->send_waitset, handle);
        assert_disabled(err_is_ok(err)); // shouldn't fail
#ifndef NDEBUG
        lc->next = lc->prev = NULL;
#endif
        if (next == first) {
            break; // wrapped
        }
    }

    dp->lmp_send_events_list = NULL;
}
