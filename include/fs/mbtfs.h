//
//  mbtfs.h
//  DoritOS
//

#ifndef mbtfs_h
#define mbtfs_h

#include <aos/aos.h>

typedef void *mbtfs_handle_t;
typedef void *mbtfs_mount_t;

errval_t mbtfs_open(void *st, char *path, mbtfs_handle_t *ret_handle);

errval_t mbtfs_create(void *st, char *path, mbtfs_handle_t *ret_handle);

errval_t mbtfs_remove(void *st, char *path);

errval_t mbtfs_read(void *st, mbtfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_read);

errval_t mbtfs_write(void *st, mbtfs_handle_t handle, void *buffer, size_t bytes, size_t *bytes_written);

errval_t mbtfs_truncate(void *st, mbtfs_handle_t handle, size_t bytes);

errval_t mbtfs_tell(void *st, mbtfs_handle_t handle, size_t *pos);

errval_t mbtfs_stat(void *st, mbtfs_handle_t handle, struct fs_fileinfo *info);

errval_t mbtfs_seek(void *st, mbtfs_handle_t handle, enum fs_seekpos whence, off_t offset);

errval_t mbtfs_close(void *st, mbtfs_handle_t handle);

errval_t mbtfs_opendir(void *st, char *path, mbtfs_handle_t *ret_handle);

//vfs_readdir(mount, h, name, NULL);
errval_t mbtfs_dir_read_next(void *st, mbtfs_handle_t handle, char **retname, struct fs_fileinfo *info);

errval_t mbtfs_closedir(void *st, mbtfs_handle_t dirhandle);

errval_t mbtfs_mkdir(void *st, char *path);

errval_t mbtfs_rmdir(void *st, char *path);

errval_t mbtfs_mount(const char *uri, mbtfs_mount_t *retst);

#endif /* mbtfs_h */
