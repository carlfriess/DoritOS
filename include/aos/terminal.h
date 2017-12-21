
#ifndef terminal_h
#define terminal_h

#define URPC_MessageType_TerminalWrite              URPC_MessageType_User0
#define URPC_MessageType_TerminalWriteLock          URPC_MessageType_User1
#define URPC_MessageType_TerminalWriteUnlock        URPC_MessageType_User2
#define URPC_MessageType_TerminalRead               URPC_MessageType_User3
#define URPC_MessageType_TerminalReadLock           URPC_MessageType_User4
#define URPC_MessageType_TerminalReadUnlock         URPC_MessageType_User5

struct terminal_msg {
    errval_t err;
    char c;
};

enum Terminal_IO {
    Terminal_Read,
    Terminal_Write
};

errval_t lock_terminal(struct aos_rpc *chan, enum Terminal_IO, bool lock);
size_t terminal_write(const char* buf, size_t len);
size_t terminal_read(char *buf, size_t len);

#endif
