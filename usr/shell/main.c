#include <stdlib.h>
#include <stdio.h>

#include <aos/aos_rpc.h>
#include <spawn/spawn.h>

#define BUF_SIZE    256

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
    printf("\t help - show list of commands\n");
    printf("\t echo");
}

static void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
    }
    printf("\n");

}

int main(int argc, char *argv[]) {

    printf("  _____     ____\n");
    printf(" /      \\  |  o |\n");
    printf("|        |/ ___\\|\n");
    printf("|_________/\n");
    printf("|_|_| |_|_|\n");
    printf("Welcome to TurtleSHELL\n\n");

    errval_t err;

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

        len = aos_rpc_terminal_read(buf, BUF_SIZE);

        num_args = parse_args(args, len, buf, argstring);

        if (num_args > 0) {
            if (!strcmp(args[0], "help")) {
                cmd_help();
            } else if (!strcmp(args[0], "echo")) {
                cmd_echo(num_args, args);
            } else {
                // Allocate spawninfo
                struct spawninfo *si = (struct spawninfo *) malloc(sizeof(struct spawninfo));

                // Spawn process
                domainid_t pid;
                err = aos_rpc_process_spawn(aos_rpc_get_init_channel(), args[0], 0, &pid);
                if (err_is_fail(err)) {
                    printf("%s: command not found\n", args[0]);
                }

                // Free spawninfo
                free(si);
            }
        }
    }

    return 0;
}