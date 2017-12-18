//
//  fs_fat_serv.h
//  DoritOS
//

#ifndef fs_fat_serv_h
#define fs_fat_serv_h

#include <aos/aos.h>

#include <fs/fs_fat.h>

#define GET_BYTES1(buf, offset) (uint8_t)   ((uint8_t *) buf)[offset]

#define GET_BYTES2(buf, offset) (uint16_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8

#define GET_BYTES3(buf, offset) (uint32_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 & \
0x00FFFFFF

#define GET_BYTES4(buf, offset) (uint32_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 | \
((uint8_t *) buf)[offset+3] << 24

#define GET_BYTES5(buf, offset) (uint64_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 | \
((uint8_t *) buf)[offset+3] << 24 | \
((uint8_t *) buf)[offset+4] << 32 & \
0x000000FFFFFFFFFF

#define GET_BYTES6(buf, offset) (uint64_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 | \
((uint8_t *) buf)[offset+3] << 24 | \
((uint8_t *) buf)[offset+4] << 32 | \
((uint8_t *) buf)[offset+5] << 40 & \
0x0000FFFFFFFFFFFF

#define GET_BYTES7(buf, offset) (uint64_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 | \
((uint8_t *) buf)[offset+3] << 24 | \
((uint8_t *) buf)[offset+4] << 32 | \
((uint8_t *) buf)[offset+5] << 40 | \
((uint8_t *) buf)[offset+6] << 48 & \
0x00FFFFFFFFFFFFFF

#define GET_BYTES8(buf, offset) (uint64_t)  ((uint8_t *) buf)[offset] | \
((uint8_t *) buf)[offset+1] << 8 | \
((uint8_t *) buf)[offset+2] << 16 | \
((uint8_t *) buf)[offset+3] << 24 | \
((uint8_t *) buf)[offset+4] << 32 | \
((uint8_t *) buf)[offset+5] << 40 | \
((uint8_t *) buf)[offset+6] << 48 | \
((uint8_t *) buf)[offset+7] << 56



struct DIR_Entry {
    
    char Name[12];      // NULL terminated
    uint8_t Attr;
    
    uint8_t NTRes;
    
    uint8_t CrtTimeTenth;
    
    uint16_t WrtTime;
    uint16_t WrtDate;
    
    uint32_t FstClus;
    uint32_t FileSize;
    
};

struct fat32_mount {
    
    struct fat_dirent *root;
    
};

errval_t init_BPB(void);

uint32_t getFATEntry(size_t n);

errval_t setFATEntry(size_t n, uint32_t value);

errval_t fat_open(void *st, char *path, struct fat_dirent **ret_dirent);

// FIXME: Add flags
errval_t fat_resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent);

// FIXME: char *name can be null terminated, but has to have the same first 11 bytes
errval_t fat_find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent);


// FIXME: Add flags
//errval_t resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent);

// FIXME: char *name can be null terminated, but has to have the same first 11 bytes
//errval_t find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent);

// LEGACY
//int filetab_alloc(struct filetab_entry *new_entry);
//struct filetab_entry *filetab_get(int index);
//void filetab_free(int index);

errval_t init_root_dir(void *st);

errval_t remove_dirent(struct fat_dirent *dirent);

errval_t remove_entry(size_t cluster_nr, size_t pos);

errval_t read_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_read);
errval_t write_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_written);

errval_t read_cluster(size_t cluster_nr, void *buffer);
errval_t write_cluster(size_t cluster_nr, void *buffer);

errval_t read_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes);
errval_t write_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes);

#endif /* fs_fat_serv_h */
