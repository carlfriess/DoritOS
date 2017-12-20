#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <aos/aos_rpc.h>
#include <aos/urpc.h>
#include <spawn/spawn.h>
#include <fs/dirent.h>
#include <fs/fs.h>

#define BUF_SIZE    256

static char *cwd;

static size_t parse_args(char *argv[], size_t len, char *in, char* out) {

    size_t argc = 0;
    size_t j = 0;
    bool escaped = 0;
    char quote = '\0';

    argv[argc++] = out;

    for (size_t i = 0; i < len && argc < MAX_CMDLINE_ARGS; i++) {
        // If escaped, copy and reset escape flag
        if (escaped) {
            out[j++] = in[i];
            escaped = !escaped;
        } else {
            // If quote is null
            if (quote == '\0') {
                // Check if quote or space or escape, else copy
                if (in[i] == '"' || in[i] == '\'') {
                    quote = in[i];
                } else if(in[i] == ' ') {
                    out[j++] = '\0';
                    argv[argc++] = argv[0] + j;
                } else if (in[i] == '\\') {
                    escaped = !escaped;
                } else {
                    out[j++] = in[i];
                }
            } else {
                // Check if matching quote, else copy
                if (in[i] == quote) {
                    quote = '\0';
                } else {
                    out[j++] = in[i];
                }
            }
        }
    }

    return argc;
}

static void cmd_help(void) {
    printf("Help Menu:\n");
    printf("\t• help - show list of commands\n");
    printf("\t• exit - quit the shell\n");
    printf("\t• echo - print out first argument\n");
}

static void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
    }
    printf("\n");

}

static void cmd_ls(size_t argc, char *argv[]) {
//    char *path;
//
//    switch (arg) {
//        case 0:
//            path = (char *) malloc(sizeof(char) * strlen(cwd));
//            strcpy(path, cwd);
//            break;
//        case 1:
//            path = (char *) malloc(sizeof(char) * (strlen(pwd) + strlen(argv[1])));
//            break;
//    }
//
//
//    free

}

static void cmd_cat(size_t argc, char *argv[]) {

}

static void cmd_mkdir(size_t argc, char *argv[]) {

}

static void cmd_rmdir(size_t argc, char *argv[]) {

}

static size_t get_input(char *buf, size_t len) {
    errval_t err = SYS_ERR_OK;
    size_t n = 0;

    struct aos_rpc *chan = aos_rpc_get_serial_channel();

    struct terminal_msg *in;
    struct terminal_msg out = {
        .lock = 1
    };

    size_t size;
    urpc_msg_type_t msg_type;

    bool in_use = 0;

    do {
        size = sizeof(struct terminal_msg);
        msg_type = URPC_MessageType_TerminalRead;

        urpc_send(chan->uc, (void *) &out, size, msg_type);
        urpc_recv_blocking(chan->uc, (void **) &in, &size, &msg_type);

        assert(msg_type == URPC_MessageType_TerminalRead);
        in_use = in->err == TERM_ERR_TERMINAL_IN_USE;
        free(in);

    } while (in_use);

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

            err = aos_rpc_serial_putchar(chan, c);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
            }
        }
    }

    size = sizeof(struct terminal_msg);
    msg_type = URPC_MessageType_TerminalReadUnlock;

    urpc_send(chan->uc, (void *) &out, size, msg_type);
    urpc_recv_blocking(chan->uc, (void **) &in, &size, &msg_type);

    free(in);

    assert(msg_type == URPC_MessageType_TerminalReadUnlock);
    return n;
}

int main(int argc, char *argv[]) {

    cwd = (char *) malloc(sizeof(char) * 2);
    cwd[0] = '/';
    cwd[1] = '\0';


    // Boot
    printf("  ______    ____\n");
    printf(" /      \\  |  o |\n");
    printf("|        |/ ___\\|\n");
    printf("|_________/\n");
    printf("|_|_| |_|_|\n");
    printf("Welcome to TurtleSHELL\n");
    printf("\n");

    char buf[BUF_SIZE];
    char argstring[BUF_SIZE];
    size_t num_args;
    char *args[MAX_CMDLINE_ARGS];

    size_t len;

    while(true) {
        printf("DoritOS: USER$ ");
        fflush(stdout);

        memset(args, 0, MAX_CMDLINE_ARGS);
        memset(argstring, 0, BUF_SIZE);
        memset(buf, 0, BUF_SIZE);

        len = get_input((char *) buf, BUF_SIZE);
        printf("\n");

        num_args = (len == 0) ? 0 : parse_args(args, len, buf, argstring);

        if (num_args > 0) {
            
            bool wait = true;
            
            if (!strcmp(args[num_args - 1], "&")) {
                wait = false;
                num_args--;
            }
            
            if (!strcmp(args[0], "help")) {
                cmd_help();
            } else if (!strcmp(args[0], "exit")) {
                printf("Exiting shell. Goodbye!\n");
                break;
            } else if (!strcmp(args[0], "echo")) {
                cmd_echo(num_args, args);
            } else if (!strcmp(args[0], "ls")) {
                cmd_ls(num_args, args);
            } else if (!strcmp(args[0], "cat")) {
                cmd_cat(num_args, args);
            } else if (!strcmp(args[0], "mkdir")) {
                cmd_mkdir(num_args, args);
            } else if (!strcmp(args[0], "rmdir")) {
                cmd_rmdir(num_args, args);
            } else {
                // Allocate spawninfo
                struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));

                // Spawn process
                domainid_t pid;
                errval_t err = aos_rpc_process_spawn(aos_rpc_get_init_channel(), args[0], 1, &pid);
                if (err_is_fail(err)) {
                    printf("%s: command not found\n", args[0]);
                } else if (wait) {
                    aos_rpc_process_deregister_notify(pid);
                }

                // Free spawninfo
                free(si);
            }
        } else {
            printf("\n");
        }
    }

    return 0;
}
