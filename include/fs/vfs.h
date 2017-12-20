//
//  vfs.h
//  DoritOS
//

#ifndef vfs_h
#define vfs_h

#include <fs/fs.h>

enum fs_type {
    RAMFS,
    FATFS,
    MULTIBOOTFS
};

struct mount_node {
    char *name;
    size_t len;
    enum fs_type type;
    struct mount_node *next;
};

struct vfs_handle {
    void *handle;
    enum fs_type type;
};

struct vfs_mount {
    void *ram_mount;
    void *fat_mount;

    struct mount_node *head;
};

typedef void *vfs_handle_t;

enum fs_type find_mount_type(struct mount_node *head, const char *path, char **ret_path);

errval_t vfs_open(void *st, const char *path, vfs_handle_t *rethandle);

errval_t vfs_create(void *st, const char *path, vfs_handle_t *rethandle);

errval_t vfs_remove(void *st, const char *path);

errval_t vfs_read(void *st, vfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read);

errval_t vfs_write(void *st, vfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_written);

errval_t vfs_truncate(void *st, vfs_handle_t handle, size_t bytes);

errval_t vfs_tell(void *st, vfs_handle_t handle, size_t *pos);

errval_t vfs_stat(void *st, vfs_handle_t handle, struct fs_fileinfo *info);

errval_t vfs_seek(void *st, vfs_handle_t handle, enum fs_seekpos whence, off_t offset);

errval_t vfs_close(void *st, vfs_handle_t handle);


errval_t vfs_opendir(void *st, const char *path, vfs_handle_t *rethandle);

//vfs_readdir(mount, h, name, NULL);
errval_t vfs_dir_read_next(void *st, vfs_handle_t handle, char **retname, struct fs_fileinfo *info);

errval_t vfs_closedir(void *st, vfs_handle_t dirhandle);


errval_t vfs_mkdir(void *st, const char *path);

errval_t vfs_rmdir(void *st, const char *path);


//errval_t vfs_mount(const char *uri, vfs_handle_t *retst);
//errval_t ramfs_mount(const char *uri, ramfs_mount_t *retst);

#endif /* vfs_h */
