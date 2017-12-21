#include <stdlib.h>
#include <stdio.h>

#include <aos/aos_rpc.h>
#include <aos/urpc.h>

#include <aos/terminal.h>

errval_t lock_terminal(struct aos_rpc *chan, enum Terminal_IO io, bool lock) {
    errval_t err;

    size_t size = sizeof(struct terminal_msg);
    urpc_msg_type_t msg_type;
    struct terminal_msg out;

    if (lock) {
        if (io == Terminal_Read) {
            msg_type = URPC_MessageType_TerminalReadLock;
        } else {
            msg_type = URPC_MessageType_TerminalWriteLock;
        }
    } else {
        if (io == Terminal_Read) {
            msg_type = URPC_MessageType_TerminalReadUnlock;
        } else {
            msg_type = URPC_MessageType_TerminalWriteUnlock;
        }
    }

    err = urpc_send(chan->uc, (void *) &out, size, msg_type);
    if (err_is_fail(err)) {
        return err;
    }

    struct terminal_msg *in;

    urpc_msg_type_t msg_type_in;

    err = urpc_recv_blocking(chan->uc, (void **) &in, &size, &msg_type_in);
    if (err_is_fail(err)) {
        return err;
    }
    assert(msg_type == msg_type_in);

    if (err_is_fail(in->err)) {
        return in->err;
    }

    free(in);

    return SYS_ERR_OK;
}

size_t terminal_write(const char* buf, size_t len) {
    errval_t err;
    size_t n = 0;

    struct aos_rpc *chan = aos_rpc_get_serial_channel();

    if (chan == NULL) {
        sys_print(buf, len);
        return len;
    }

    err = lock_terminal(chan, Terminal_Write, 1);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    {
        for (size_t i = 0; i < len; i++) {

            err = aos_rpc_serial_putchar(chan, buf[i]);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }

            n = i + 1;
        }
    }

    err = lock_terminal(chan, Terminal_Write, 0);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    return n;
}

size_t terminal_read(char *buf, size_t len) {
    errval_t err;
    size_t n = 0;

    struct aos_rpc *chan = aos_rpc_get_serial_channel();

    if (chan == NULL) {
        for (size_t i = 0; i < len; i++) {
            sys_getchar(&buf[i]);
            n = i;
        }
        return n;
    }

    err = lock_terminal(chan, Terminal_Read, 1);

    {
        for (size_t i = 0; i < len; i++) {

            char c;

            err = aos_rpc_serial_getchar(chan, &c);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }

            if (c == 0x04 || c == 0x0A || c == 0x0D) {
                break;
            }
            n = i + 1;
            buf[i] = c;
        }
    }

    err = lock_terminal(chan, Terminal_Read, 0);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }

    return n;
}