//
//  fs_rpc.h
//  DoritOS
//

#ifndef fs_rpc_h
#define fs_rpc_h

#include <aos/aos.h>

#include <fs/fs.h>
#include <fs/fs_fat.h>

#define UMP_MessageType_Open      UMP_MessageType_User0
#define UMP_MessageType_Close     UMP_MessageType_User1
#define UMP_MessageType_Read      UMP_MessageType_User2
#define UMP_MessageType_Write     UMP_MessageType_User3
#define UMP_MessageType_Add       UMP_MessageType_User4
#define UMP_MessageType_Remove    UMP_MessageType_User5
#define UMP_MessageType_Create    UMP_MessageType_User6

struct fat32fs_handle
{
    //struct fs_handle common;      // TODO: What is this for?
    char *path;
    bool isdir;
    struct fat_dirent *dirent;
    off_t file_pos;
};

struct fs_message {
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    
    
};

errval_t fs_rpc_init(void *state);

errval_t fs_rpc_open(void *st, const char *path, struct fat32fs_handle **ret_handle);

errval_t fs_rpc_create(void *st, const char *path, struct fat32fs_handle **ret_handle);

errval_t fs_rpc_remove(void *st, const char *path);

errval_t fs_rpc_read(void *st, struct fat32fs_handle *handle, void *buffer, size_t bytes, size_t *bytes_read);

errval_t fs_rpc_write(void *st, struct fat32fs_handle *handle, const void *buffer, size_t bytes, size_t *bytes_written);

errval_t fs_rpc_tell(void *st, struct fat32fs_handle *handle, size_t *pos);

errval_t fs_rpc_stat(void *st, struct fat32fs_handle *inhandle, struct fs_fileinfo *info);

errval_t fs_rpc_seek(void *st, struct fat32fs_handle *handle, enum fs_seekpos whence, off_t offset);

errval_t fs_rpc_close(void *st, struct fat32fs_handle *inhandle);

/*
errval_t fat32fs_open(void *st, const char *path, fat32fs_handle_t *rethandle)

errval_t fat32fs_create(void *st, const char *path, fat32fs_handle_t *rethandle);

//errval_t fat32fs_remove(void *st, const char *path);

errval_t fat32fs_read(void *st, ramfs_handle_t handle, void *buffer, size_t bytes,
                      size_t *bytes_read);
*/


#endif /* fs_rpc_h */
