//
//  fatfs_serv.h
//  DoritOS
//

#ifndef fatfs_serv_h
#define fatfs_serv_h

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

struct fatfs_serv_mount {
    
    struct fat_dirent *root;
    
};

errval_t init_BPB(void);

uint32_t getFATEntry(size_t n);

errval_t setFATEntry(size_t n, uint32_t value);

 // TODO: Differentiate between fatfs_serv_open and fatfs_serv_opendir with wrapper function!!!

errval_t fatfs_serv_open(void *st, char *path, struct fat_dirent **ret_dirent);

errval_t fatfs_serv_create(void *st, char *path, struct fat_dirent **ret_dirent);


// FIXME: Add flags
errval_t fat_resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent);

// FIXME: char *name can be null terminated, but has to have the same first 11 bytes
errval_t fat_find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent);

// FIXME: Add flags
//errval_t resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent);

// FIXME: char *name can be null terminated, but has to have the same first 11 bytes
//errval_t find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent);


errval_t init_root_dir(void *st);

errval_t fatfs_rm_serv(void *st, char *path);

errval_t remove_dirent(struct fat_dirent *dirent);



// FAT HELPER FUNCTIONS

uint32_t find_free_fat_entry_and_set(size_t start_search_entry, uint32_t value);


errval_t remove_fat_entries(size_t cluster_nr);

errval_t remove_fat_entries_from(size_t cluster_nr, size_t start_index);




errval_t append_cluster_chain(size_t cluster_nr, size_t cluster_count);


// DIRECTORY FUNCTIONS

errval_t fatfs_serv_opendir(void *st, char *path, struct fat_dirent **ret_dirent);

errval_t fatfs_serv_readdir(size_t cluster_nr, size_t dir_index, struct fat_dirent **ret_dirent);



// DIRECTORY ENTRY HELPER FUNCTIONS

errval_t get_dir_entry(size_t cluster_nr, size_t pos, struct DIR_Entry **ret_dir_data);

errval_t set_dir_entry(size_t cluster_nr, size_t pos, struct DIR_Entry *dir_data);

errval_t add_dir_entry(size_t cluster_nr, struct DIR_Entry *dir_data, size_t *ret_pos);

errval_t remove_dir_entry(size_t cluster_nr, size_t pos);



errval_t get_dir_entry_size(size_t cluster_nr, size_t pos, size_t *ret_size);

errval_t set_dir_entry_size(size_t cluster_nr, size_t pos, size_t file_size);

errval_t add_dir_entry_size(size_t cluster_nr, size_t pos, int delta, size_t *ret_size);



errval_t get_dir_entries_count(size_t cluster_nr, size_t *ret_count);

errval_t find_free_dir_entry(size_t cluster_nr, size_t *ret_pos);



errval_t fatfs_serv_mkdir(void *st, char *path);

errval_t fatfs_serv_rmdir(void *st, char *path);


errval_t truncate_dirent(struct fat_dirent *dirent, size_t bytes);


errval_t read_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_read);
errval_t write_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_written);


errval_t read_cluster(size_t cluster_nr, void *buffer);
errval_t write_cluster(size_t cluster_nr, void *buffer);

errval_t read_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes);
errval_t write_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes);


struct fat_dirent *create_dirent(char *name, size_t first_cluster_nr, size_t size, bool is_dir,
                                 size_t parent_cluster_nr, size_t parent_pos);



#endif /* fs_fat_serv_h */
