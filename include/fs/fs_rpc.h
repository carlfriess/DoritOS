//
//  fs_rpc.h
//  DoritOS
//

#ifndef fs_rpc_h
#define fs_rpc_h

#include <aos/aos.h>

#include <fs/fs.h>
#include <fs/fs_fat.h>
#include <fs/dirent.h>

#define UMP_MessageType_Open      UMP_MessageType_User0
#define UMP_MessageType_Close     UMP_MessageType_User1
#define UMP_MessageType_Read      UMP_MessageType_User2
#define UMP_MessageType_Write     UMP_MessageType_User3
#define UMP_MessageType_Truncate  UMP_MessageType_User4
#define UMP_MessageType_Remove    UMP_MessageType_User5
#define UMP_MessageType_Create    UMP_MessageType_User6

#define UMP_MessageType_OpenDir   UMP_MessageType_User7
#define UMP_MessageType_CloseDir  UMP_MessageType_User8
#define UMP_MessageType_ReadDir   UMP_MessageType_User9
#define UMP_MessageType_MakeDir   UMP_MessageType_User10
#define UMP_MessageType_RemoveDir UMP_MessageType_User11

typedef void *fat32fs_handle_t;

struct fat32fs_handle
{
    //struct fs_handle common;      // TODO: What is this for?
    char *path;
    bool isdir;
    struct fat_dirent *dirent;
    off_t pos;
};

struct fs_message {
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    
};

errval_t fs_rpc_init(void *state);

errval_t fs_rpc_open(void *st, char *path, fat32fs_handle_t *ret_handle);

errval_t fs_rpc_create(void *st, char *path, fat32fs_handle_t *ret_handle);

errval_t fs_rpc_remove(void *st, char *path);

errval_t fs_rpc_read(void *st, fat32fs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read);

errval_t fs_rpc_write(void *st, fat32fs_handle_t handle, const void *buffer, size_t bytes, size_t *bytes_written);

errval_t fs_rpc_truncate(void *st, fat32fs_handle_t handle, size_t bytes);

errval_t fs_rpc_tell(void *st, fat32fs_handle_t handle, size_t *ret_pos);

errval_t fs_rpc_stat(void *st, fat32fs_handle_t inhandle, struct fs_fileinfo *info);

errval_t fs_rpc_seek(void *st, fat32fs_handle_t handle, enum fs_seekpos whence, off_t offset);

errval_t fs_rpc_close(void *st, fat32fs_handle_t inhandle);

errval_t fs_rpc_opendir(void *st, char *path, fs_dirhandle_t *ret_handle);

errval_t fs_rpc_readdir(void *st, fs_dirhandle_t dirhandle, char **ret_name, struct fs_fileinfo *info);

errval_t fs_rpc_closedir(void *st, fs_dirhandle_t dirhandle);

errval_t fs_rpc_mkdir(void *st, char *path);

errval_t fs_rpc_rmdir(void *st, char *path);

/*
errval_t fat32fs_open(void *st, const char *path, fat32fs_handle_t *rethandle)

errval_t fat32fs_create(void *st, const char *path, fat32fs_handle_t *rethandle);

//errval_t fat32fs_remove(void *st, const char *path);

errval_t fat32fs_read(void *st, ramfs_handle_t handle, void *buffer, size_t bytes,
                      size_t *bytes_read);
*/


#endif /* fs_rpc_h */
