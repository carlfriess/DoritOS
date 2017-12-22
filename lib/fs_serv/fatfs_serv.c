//
//  fatfs_serv.c
//  DoritOS
//

#include <stdio.h>
#include <string.h>

#include <aos/aos.h>

#include <fs_serv/fat_helper.h>

#include <fs_serv/fatfs_serv.h>

#define PRINT_DEBUG 0

extern errval_t mmchs_read_block(size_t block_nr, void *buffer);
extern errval_t mmchs_write_block(size_t block_nr, void *buffer);

static void get_dot_dir_data(size_t cluster_nr, struct DIR_Entry *dir_data);
static void get_dot_dot_dir_data(size_t cluster_nr, struct DIR_Entry *dir_data);


static void data_to_dir_data(struct DIR_Entry *dest, void *src);
static void dir_data_to_data(void *dest, struct DIR_Entry *src);

errval_t init_BPB(void) {
    
    // Boot Sector and BPB Structure
    
    errval_t err;
    
    uint8_t *data = calloc(512, sizeof(uint8_t));
    
    err = mmchs_read_block(0, data);
    if (err_is_fail(err)) {
        assert(err_is_ok(err));
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    BPB_BytsPerSec = GET_BYTES2(data, 11);
    //debug_printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
    
    BPB_SecPerClus = GET_BYTES1(data, 13);
    //debug_printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
    
    BPB_ResvdSecCnt = GET_BYTES2(data, 14);
    //debug_printf("BPB_ResvdSecCnt: %d\n", BPB_ResvdSecCnt);
    
    BPB_NumFATs = GET_BYTES1(data, 16);
    //debug_printf("BPB_NumFATs: %d\n", BPB_NumFATs);
    
    BPB_RootEntCnt = GET_BYTES2(data, 17);
    //debug_printf("BPB_RootEntCnt: %d\n", BPB_RootEntCnt);
    
    BPB_TotSec16 = GET_BYTES2(data, 19);
    //debug_printf("BPB_TotSec16: %d\n", BPB_TotSec16);
    
    BPB_Media = GET_BYTES1(data, 21);
    //debug_printf("BPB_Media: %x\n", BPB_Media);
    
    BPB_FATSz16 = GET_BYTES2(data, 22);
    //debug_printf("BPB_FATSz16: %d\n", BPB_FATSz16);
    
    BPB_SecPerTrk = GET_BYTES2(data, 24);
    //debug_printf("BPB_SecPerTrk: %d\n", BPB_SecPerTrk);
    
    BPB_NumHeads = GET_BYTES2(data, 26);
    //debug_printf("BPB_NumHeads: %d\n", BPB_NumHeads);
    
    BPB_HiddSec = GET_BYTES4(data, 28);
    //debug_printf("BPB_HiddSec: %d\n", BPB_HiddSec);
    
    BPB_TotSec32 = GET_BYTES4(data, 32);
    //debug_printf("BPB_TotSec32: %d\n", BPB_TotSec32);
    
    BPB_FATSz32 = GET_BYTES4(data, 36);
    //debug_printf("BPB_FATSz32: %d\n", BPB_FATSz32);
    
    BPB_ExtFlags = GET_BYTES2(data, 40);
    //debug_printf("BPB_ExtFlags: %x\n", BPB_ExtFlags);
    
    BPB_FSVer = GET_BYTES2(data, 42);
    //debug_printf("BPB_FSVer: %x\n", BPB_FSVer);
    
    BPB_RootClus = GET_BYTES4(data, 44);
    //debug_printf("BPB_RootClus: %d\n", BPB_RootClus);
    
    BPB_FSInfo = GET_BYTES2(data, 48);
    //debug_printf("BPB_FSInfo: %d\n", BPB_FSInfo);
    
    BPB_BkBootSec = GET_BYTES2(data, 50);
    //debug_printf("BPB_BkBootSec: %d\n", BPB_BkBootSec);
    
    memcpy(BPB_Reserved, &data[52], 12);
    //debug_printf("BPB_Reserved: ");
    //for (int i = 0; i < 12; ++i) {
    //    printf("%02x ", BPB_Reserved[i]);
    //}
    //printf("\n");
    
    BS_DrvNum = GET_BYTES1(data, 64);
    //debug_printf("BS_DrvNum: %d\n", BS_DrvNum);
    
    BS_Reserved1 = GET_BYTES1(data, 65);
    //debug_printf("BS_Reserved1: %d\n", BS_Reserved1);
    
    BS_BootSig = GET_BYTES1(data, 66);
    //debug_printf("BS_BootSig: %d\n", BS_BootSig);
    
    BS_VolID = GET_BYTES4(data, 67);
    //debug_printf("BS_VolID: %x\n", BS_VolID);
    
    memcpy(BS_VolLab, &data[71], 11);
    BS_VolLab[11] = '\0';
    //debug_printf("BS_VolLab: %s\n", BS_VolLab);
    
    memcpy(BS_FilSysType, &data[82], 8);
    BS_FilSysType[8] = '\0';
    //debug_printf("BS_FilSysType: %s\n", BS_FilSysType);
    
    // Calculating count of sectors occupied by the root directory
    RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec - 1)) / BPB_BytsPerSec;      // Always 0 in FAT32
    
    // Calculating actual FAT size
    if (BPB_FATSz16 != 0) {
        FATSz = BPB_FATSz16;
    } else {
        FATSz = BPB_FATSz32;
    }
    
    // Calculating start of data region (first sector of cluster 2)
    FirstDataSector = BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors;
 
    free(data);
    
    return err;
    
}

inline static uint32_t getFirstSectorOfCluster(size_t n) {
    
    return  ((n - 2) * BPB_SecPerClus) + FirstDataSector;
    
}

uint32_t getFATEntry(size_t n) {
    
    errval_t err;
    
    // Number of FAT entries that fit into one sector
    size_t FATEntPerSec = BPB_BytsPerSec/sizeof(uint32_t);
    
    // FAT sector that contains nth entry
    size_t ThisFATSecNum = BPB_ResvdSecCnt + (n / FATEntPerSec);
    
    // FAT entry in that sector
    size_t ThisFATEnt = n % FATEntPerSec;
    
    // Allocate buffer for FAT sector
    uint32_t *buffer = calloc(FATEntPerSec, sizeof(uint32_t));
    
    // Read FAT sector
    err = mmchs_read_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        free(buffer);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    uint32_t value = buffer[ThisFATEnt] & 0x0FFFFFFF;
    
    free(buffer);
    
    // Return entry in FAT with MSB masked out
    return value;
    
}


errval_t setFATEntry(size_t n, uint32_t value) {
 
    errval_t err;
 
    // Number of FAT entries that fit into one sector
    size_t FATEntPerSec = BPB_BytsPerSec/sizeof(uint32_t);

    // FAT sector that contains nth entry
    size_t ThisFATSecNum = BPB_ResvdSecCnt + (n / FATEntPerSec);
 
    // FAT entry in that sector
    size_t ThisFATEnt = n % FATEntPerSec;

    // Allocate buffer for FAT sector
    uint32_t *buffer = calloc(FATEntPerSec, sizeof(uint32_t));

    // Read FAT sector of SD card
    err = mmchs_read_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        free(buffer);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
 
    // Change FAT entry to value without MSB
    buffer[ThisFATEnt] = buffer[ThisFATEnt] & 0xF0000000;
    buffer[ThisFATEnt] = buffer[ThisFATEnt] | value;
 
    // Write FAT sector back to SD card
    err = mmchs_write_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        free(buffer);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    free(buffer);
 
    return err;
 
}

errval_t fatfs_serv_open(void *st, char *path, struct fat_dirent **ret_dirent) {

#if PRINT_DEBUG
    debug_printf("Opening file: -%s-\n", path);
#endif
    
    errval_t err;
    
    assert(ret_dirent != NULL);
    
    struct fatfs_serv_mount *mount = st;
    
    // Allocate memory for potential new file dirent
    struct fat_dirent *file_dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Resolve path to find file dirent
    err = fat_resolve_path(mount->root, path, &file_dirent);
    if (err_is_fail(err)) {
        free(file_dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Set return dirent
    *ret_dirent = file_dirent;
    
    return err;
    
}



errval_t fatfs_serv_create(void *st, char *path, struct fat_dirent **ret_dirent) {

#if PRINT_DEBUG
    debug_printf("Creating file: -%s-\n", path);
#endif
    
    errval_t err;
    
    assert(ret_dirent != NULL);

    struct fatfs_serv_mount *mount = st;
    
    // Allocate memory for potential new file dirent
    struct fat_dirent *file_dirent = calloc(1, sizeof(struct fat_dirent));

    err = fat_resolve_path(mount->root, path, &file_dirent);
    if (err_is_ok(err)) {
        // Free file dirent
        free(file_dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(FS_ERR_EXISTS));
#endif
        return FS_ERR_EXISTS;
    }
    
    // Free file dirent
    free(file_dirent);

    /////
    struct fat_dirent *parent = calloc(1, sizeof(struct fat_dirent));
    memcpy(parent, mount->root, sizeof(struct fat_dirent));
    char *childname;
    
    // Find parent directory
    char *lastsep = strrchr(path, FS_PATH_SEP);
    
    if (lastsep != NULL) {
        
        childname = lastsep + 1;
        
        size_t pathlen = lastsep - path;
        char pathbuf[pathlen + 1];
        memcpy(pathbuf, path, pathlen);
        pathbuf[pathlen] = '\0';
        
        // Resolve parent directory
        err = fat_resolve_path(mount->root, pathbuf, &parent);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        } else if (parent != NULL && !parent->is_dir) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
#endif
            return FS_ERR_NOTDIR; // parent is not a directory
        }
        
        assert(mount->root != NULL);
        
        
    } else {
        
        childname = path;
        
    }

    // Insert into directory
    
    // Search for a free FAT entry and claim it
    size_t file_cluster_nr = find_free_fat_entry_and_set(0, 0x0FFFFFFF);

#if PRINT_DEBUG
    debug_printf("file_cluster_nr: %zu\n", file_cluster_nr);
#endif
    
    // Convert name to fat name
    char *fat_name = convert_to_fat_name(childname);
    
    // Construct DIR_Entry
    struct DIR_Entry *dir_data = calloc(1, sizeof(struct DIR_Entry));
    
    memcpy(dir_data->Name, fat_name, 12);
    //dir_data->NTRes = 0;
    dir_data->CrtTimeTenth = 0;
    dir_data->WrtTime = 0;
    dir_data->WrtDate = 0;
    dir_data->FstClus = file_cluster_nr;
    dir_data->FileSize = 0;
    
    // Free fat name
    free(fat_name);
    
    // First cluster number of parent
    size_t parent_cluster_nr;
    
    // Depending on parent set parents cluster number
    if (parent != NULL) {
        
        // Set parent cluster number as parent directory cluster number
        parent_cluster_nr = parent->first_cluster_nr;
        
    } else {
    
        assert(mount->root != NULL);
        
        // Set parent cluster number as root directory cluster number
        parent_cluster_nr = mount->root->first_cluster_nr;

    }
    
    // Position in parent where entry is inserted
    size_t parent_pos = 0;
    
    // Add directory entry to parent directory
    err = add_dir_entry(parent_cluster_nr, dir_data, &parent_pos);
    if (err_is_fail(err)) {
        free(dir_data);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Free DIR_Entry
    free(dir_data);
    
    // Set return dirent
    *ret_dirent = create_dirent(childname, file_cluster_nr, 0, false, parent_cluster_nr, parent_pos);

    return err;
    
}


errval_t fat_resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent) {
    
    errval_t err = SYS_ERR_OK;
#if PRINT_DEBUG
    debug_printf("Resolve %s path\n", path);
#endif
    assert(ret_dirent != NULL);
    
    // Skip leading '/'
    size_t pos = 0;
    if (path[0] == FS_PATH_SEP) {
        pos++;
    }

    // Current dirent (alloacted and content of root copied in)s
    struct fat_dirent *curr_dirent = calloc(1, sizeof(struct fat_dirent));
    memcpy(curr_dirent, root, sizeof(struct fat_dirent));
    
    // Next dirent (actual struct is being allocated by fat_find_dirent every iteration)
    struct fat_dirent *next_dirent = NULL;
    
    while (path[pos] != '\0') {
        
        // Set nextsep pointer to position of next / after &path[pos]
        char *nextsep = strchr(&path[pos], FS_PATH_SEP);
        
        // Next name length
        size_t nextlen;
        
        // Check wether this is the last part of path
        if (nextsep == NULL) {
            nextlen = strlen(&path[pos]);
        } else {
            nextlen = nextsep - &path[pos];
        }
        
        // Copy name of this layer in pathbuf
        char pathbuf[nextlen + 1];
        memcpy(pathbuf, &path[pos], nextlen);
        pathbuf[nextlen] = '\0';
#if PRINT_DEBUG
        debug_printf("PATHBUF %s\n", pathbuf);
#endif
        
        // Find directory entry with same name as in pathbuf
        err = fat_find_dirent(curr_dirent, pathbuf, &next_dirent);
        if (err_is_fail(err)) {
            
            // Free both current dirent
            free(curr_dirent);
            // Free when next dirent if previously allocated
            if (next_dirent != NULL) {
                free(next_dirent);
            }
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Check if middle level and not a directory
        if (!next_dirent->is_dir && nextsep != NULL) {
            // Free both current and next dirent
            free(curr_dirent);
            free(next_dirent);
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
#endif
            return FS_ERR_NOTDIR;
        }
        
        // Free current dirent since not needed anymore
        free(curr_dirent);
        
        // Set current dirent as next_dirent
        curr_dirent = next_dirent;
        if (nextsep == NULL) {
            break;
        }

        // Update pos
        pos += nextlen + 1;
        
    }
    
    // Set return dirent as next dirent (struct was allocated in fat_find_dirent)
    *ret_dirent = next_dirent;
    
    return err;
    
}

errval_t fat_find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent) {
    
    errval_t err;

    assert(curr_dirent != NULL);
    assert(ret_dirent != NULL);
    
    // Converts name into fat directory name format (with max 8 + 3 size)
    char *fat_name = convert_to_fat_name(name);
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Temporal cluster number
    size_t temp_nr = curr_dirent->first_cluster_nr;
    
    // Indicates if End Of File is reached
    bool isEOF = false;
    
    // Data buffer for the data of one cluster
    uint8_t *data = calloc(1, BytesPerClus);
    
    for (int cluster_index = 0; !isEOF; cluster_index++) {
        
        // Read data from cluster[temp_nr] in data section
        err = read_cluster(temp_nr , (void *) data);
        if (err_is_fail(err)) {
            // Free converted FAT directory entry name
            free(fat_name);
            // Free data buffer
            free(data);
            
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        for (int pos_index = 0; pos_index < BytesPerClus / 32; pos_index++) {
            
            uint8_t *dirent_data = &data[pos_index * 32];
            
            if (memcmp(fat_name, &dirent_data[0], 11) == 0) {
#if PRINT_DEBUG
                debug_printf("SAME NAME\n");
#endif
                // Allocating memory for return dirent
                struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
                
                // Set size of dirent
                dirent->size = *((uint32_t *) &dirent_data[28]);
                
                // Set name of dirent
                memcpy(&dirent->name, &dirent_data[0], 11);
                
                // Set parent directory first cluster number
                dirent->parent_cluster_nr = curr_dirent->first_cluster_nr;
                
                // Set position in parent directory
                dirent->parent_pos = (cluster_index * BytesPerClus / 32) + pos_index;
                
                // Set if dirent is a directory
                dirent->is_dir = dirent_data[11] & 0x18;
                
                // Set first cluster number of dirent
                dirent->first_cluster_nr = *((uint16_t *) &dirent_data[26]) | *((uint16_t *) &dirent_data[20]) << 16;
                
                // Return newly allocated dirent
                *ret_dirent = dirent;
                
                // Free data buffer
                free(data);
                
                return SYS_ERR_OK;
                
            }
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
            isEOF = true;
        }
        
    }
    
    // Free converted FAT directory entry name
    free(fat_name);
    
    // Free data buffer
    free(data);
    
    return FAT_ERR_FAT_LOOKUP;
    
}


/// FAT_DIRENT

struct fat_dirent *create_dirent(char *name, size_t first_cluster_nr, size_t size, bool is_dir,
                                 size_t parent_cluster_nr, size_t parent_pos) {
    
    struct fat_dirent *ret_dirent = calloc(1, sizeof(struct fat_dirent));
    
    ret_dirent->size = size;
    
    memcpy(&ret_dirent->name, name, 12);
    
    ret_dirent->parent_cluster_nr = parent_cluster_nr;
    
    ret_dirent->parent_pos = parent_pos;
    
    ret_dirent->is_dir = is_dir;
    
    ret_dirent->first_cluster_nr = first_cluster_nr;
    
    return ret_dirent;
    
}


errval_t init_root_dir(void *st) {
    
    errval_t err;
    
    struct fatfs_serv_mount *mt = st;
    
    mt->root = create_dirent("/", 2, 0, true, 0, 0);
    
    size_t count = 0;
    
    err = get_dir_entries_count(mt->root->first_cluster_nr, &count);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
    }
    
#if PRINT_DEBUG
    debug_printf("Amount of entries in the root directory: %zu\n", count);
#endif
    return SYS_ERR_OK;
    
}

// ADD AND REMOVE FOR DIRENT

errval_t remove_dirent(struct fat_dirent *dirent) {
    
    errval_t err;
    
    assert(dirent != NULL);
    
    // Remove the directory entry in parent
    err = remove_dir_entry(dirent->parent_cluster_nr, dirent->parent_pos);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Zeros out all FAT entries of dirent's cluster chain
    err = remove_fat_entries(dirent->first_cluster_nr);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    return err;
    
}

// Inclusive . and ..
errval_t get_dir_entries_count(size_t cluster_nr, size_t *ret_count) {
    
    errval_t err;
    
    assert(ret_count != NULL);
    
    // Counter of taken entries in directory
    size_t count = 0;
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Entries per cluster
    size_t EntriesPerClus = BytesPerClus / 32;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Indicates if End Of File is reached
    bool isEOF = false;
    
    // Data buffer for the data of one cluster
    uint8_t *data = calloc(BytesPerClus, sizeof(uint8_t));
    
    while (!isEOF) {
        
        // Read data from cluster[temp_nr] in data section
        err = read_cluster(temp_nr , (void *) data);
        if (err_is_fail(err)) {
            free(data);
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        for (int pos_index = 0; pos_index < EntriesPerClus; pos_index++) {
            
            uint8_t dirent_data = data[pos_index * 32];
            
            if (dirent_data == 0x00) {
                // Free entry and no more entries after this one
                isEOF = true;
                break;
            } else if (dirent_data == 0xE5) {
                // Free entry but can be smore entries after this one
                
            } else {
                // Taken entry
                count++;
            }
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
            isEOF = true;
        }
        
    }
    
    //debug_printf("Successfully looked through directory file with starting cluster %zu\n", cluster_nr);
#if PRINT_DEBUG
    debug_printf("Directory has %zu entries\n", count);
#endif
    
    free(data);
    
    // Set return count
    *ret_count = count;
    
    return err;
    
}


errval_t find_free_dir_entry(size_t cluster_nr, size_t *ret_pos) {
    
    errval_t err;
    
    assert(ret_pos != NULL);
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Entries per cluster
    size_t EntriesPerClus = BytesPerClus / 32;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Indicates if End Of File is reached
    bool isEOF = false;
    
    // Data buffer for the data of one cluster
    uint8_t *data = calloc(BytesPerClus, sizeof(uint8_t));
    
    // Cluster index
    int cluster_index;
    
    // Index position of directory entry relative to current cluster
    int pos_index;
    
    for (cluster_index = 0; !isEOF; cluster_index++) {
        
        // Read data from cluster[temp_nr] in data section
        err = read_cluster(temp_nr , (void *) data);
        if (err_is_fail(err)) {
            free(data);
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        for (pos_index = 0; pos_index < EntriesPerClus; pos_index++) {
            
            uint8_t dirent_data = data[pos_index * 32];
            
            if (dirent_data == 0x00) {
                // Free entry and no more entries after this one
                
                // Return position
                *ret_pos = (cluster_index * EntriesPerClus) + pos_index;
                
                free(data);
                
                return err;
                
            } else if (dirent_data == 0xE5) {
                // Free entry but can be smore entries after this one
                
                // Return position
                *ret_pos = (cluster_index * EntriesPerClus) + pos_index;
                
                free(data);
                
                return err;
                
            }
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
            isEOF = true;
        }
        
    }
    
    // Return position that would be free after appending the cluster chain
    *ret_pos = (cluster_index * EntriesPerClus) + pos_index;
    
    free(data);
                           
    return FS_ERR_INDEX_BOUNDS;
    
}

errval_t get_dir_entry(size_t cluster_nr, size_t pos, struct DIR_Entry **ret_dir_data) {
    
    errval_t err;
    
    assert(ret_dir_data != NULL);
    
    // Data buffer
    uint8_t data[32];
    
    // Read directory data from [pos * 32, pos * 32 + 31]
    err = read_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Convert data to return directory data
    data_to_dir_data(*ret_dir_data, data);
    
    return err;
    
}

errval_t set_dir_entry(size_t cluster_nr, size_t pos, struct DIR_Entry *dir_data) {
    
    errval_t err;
    
    uint8_t data[32];
    
    // Read directory data from [pos * 32, pos * 32 + 31]
    err = read_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Convert directory data and save it in data buffer
    dir_data_to_data(data, dir_data);
    
    // Write modified directory data buffer back
    err = write_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    return err;
    
}

errval_t add_dir_entry(size_t cluster_nr, struct DIR_Entry *dir_data, size_t *ret_pos) {
    
    errval_t err;
    
    assert(ret_pos != NULL);
    
    // Initialize position
    size_t pos = 0;

    // Find a free directory entry and give back its position
    err = find_free_dir_entry(cluster_nr, &pos);
    
    // Handle case that no free entry left in directory scluster chain by appending it
    if (err_is_fail(err)) {
        
        switch (err) {
            case FS_ERR_INDEX_BOUNDS:
                // Append cluster chain by one cluster
                err = append_cluster_chain(cluster_nr, 1);
                if (err_is_fail(err)) {
#if PRINT_DEBUG
                    debug_printf("%s\n", err_getstring(err));
#endif
                    return err;
                }
                break;
            default:
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
                return err;
        }
        
    }
    
    // Set directory entry to data at certain position
    err = set_dir_entry(cluster_nr, pos, dir_data);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Set return position
    *ret_pos = pos;
    
    return err;
    
}

errval_t fatfs_rm_serv(void *st, char *path) {
    
    errval_t err;
    
    struct fatfs_serv_mount *mount = st;
    
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Resolve path to get
    err = fat_resolve_path(mount->root, path, &dirent);
    if (err_is_fail(err)) {
        free(dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    if (dirent == NULL) {
        free(dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", FS_ERR_NOTFOUND);
#endif
        return FS_ERR_NOTFOUND;
    }
    
    if (dirent->is_dir) {
        free(dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", FS_ERR_NOTFILE);
#endif
        return FS_ERR_NOTFILE;
    }

    // Remove dirent from directory and set FAT entries to zero
    err = remove_dirent(dirent);
    if (err_is_fail(err)) {
        free(dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Free dirent
    free(dirent);
    
    return err;
    
}


errval_t remove_dir_entry(size_t cluster_nr, size_t pos) {
    
    errval_t err;
#if PRINT_DEBUG
    debug_printf("remove dir entry %zu %zu\n", cluster_nr, pos);
#endif
    // Set directory entry region [0, 10] to indicate free entry
    uint8_t empty_name_buffer[11];
    memset(empty_name_buffer, 0, 11);
    empty_name_buffer[0] = 0xE5;
    
    err = write_cluster_chain(cluster_nr, empty_name_buffer, pos * 32, 11);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
#if PRINT_DEBUG
    debug_printf("Done empty name part\n");
#endif
    // Set directory entry region [20, 31] zero
    uint8_t empty_rest_buffer[12];
    memset(empty_rest_buffer, 0, 12);
    
    err = write_cluster_chain(cluster_nr, empty_rest_buffer, pos * 32 + 20, 12);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    return err;
    
}

errval_t get_dir_entry_size(size_t cluster_nr, size_t pos, size_t *ret_size) {
    
    errval_t err;
    
    assert(ret_size != NULL);
    
    // Data buffer for directory entry (32 bytes)
    uint32_t data[8];
    
    // Read directory data from [pos * 32, pos * 32 + 31]
    err = read_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Return directory entry file size in [28, 31]
    *ret_size = data[7];
    
    return err;
    
}

errval_t set_dir_entry_size(size_t cluster_nr, size_t pos, size_t file_size) {
    
    errval_t err;
    
    // Data buffer for directory entry (32 bytes)
    uint32_t data[8];
    
    // Read directory data from [pos * 32, pos * 32 + 31]
    err = read_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Set file size in data buffer at [28, 31]
    data[7] = file_size;
    
    // Write directory data with modified file size back
    err = write_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    return err;
    
}

errval_t add_dir_entry_size(size_t cluster_nr, size_t pos, int delta, size_t *ret_size) {
    
    errval_t err;
    
    assert(ret_size != NULL);
    
    // Data buffer for directory entry (32 bytes)
    uint32_t data[8];
    
    // Read directory data from [pos * 32, pos * 32 + 31]
    err = read_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Get file size fron data buffer at [28, 31]
    size_t file_size = data[7];
    
    // Check that file size will not underflow
    assert(delta >= 0 || ((int) file_size) - delta >= 0);
    
    // Add delta to file size of data buffer at [28, 31]
    data[7] = file_size + delta;
    
    // Write directory data with modified file size back
    err = write_cluster_chain(cluster_nr, data, pos * 32, 32);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Return new file size
    *ret_size = file_size + delta;
    
    return err;

}


errval_t remove_fat_entries(size_t cluster_nr) {
    
    errval_t err = SYS_ERR_OK;
    
    // Current cluster number
    size_t curr_nr = cluster_nr;
    
    // Next cluster number
    size_t next_nr;
    
    // End of file
    bool isEOF = false;
    
    // Set cluster chain entries in FAT to zero
    while(!isEOF) {
        
        // Set next cluster number to be next cluster number of current cluster number and check if it is EOC
        if (0x0FFFFFF8 <= (next_nr = getFATEntry(curr_nr))) {
            isEOF = true;
        }
        
        // Check that current cluster number is not one of the first two special FAT entries
        if (curr_nr < 2) {
#if PRINT_DEBUG
            debug_printf("Current cluster number is in first two special FAT entries\n");
#endif
            break;
        }
        
        // Set current cluster number FAT entry to zero
        err = setFATEntry(curr_nr, 0);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Update current cluster number to next cluster number
        curr_nr = next_nr;
        
    }
    
    return err;
    
}

errval_t remove_fat_entries_from(size_t cluster_nr, size_t start_index) {
    
    errval_t err = SYS_ERR_OK;
    
    // Current cluster number
    size_t curr_nr = cluster_nr;
    
    // Next cluster number
    size_t next_nr;
    
    // End of file
    bool isEOF = false;
    
    // Set cluster chain entries in FAT to zero
    for (int cluster_index = 0; !isEOF; cluster_index++) {
        
        // Set next cluster number to be next cluster number of current cluster number and check if it is EOC
        if (0x0FFFFFF8 <= (next_nr = getFATEntry(curr_nr))) {
            isEOF = true;
        }
        
        // Check that current cluster number is not one of the first two special FAT entries
        if (curr_nr < 2) {
#if PRINT_DEBUG
            debug_printf("Current cluster number is in first two special FAT entries\n");
#endif
            break;
        }
        
        // Only start setting FAT entries to zero when we have reached
        if (start_index <= cluster_index) {
            
            // Set current cluster number FAT entry to zero
            err = setFATEntry(curr_nr, 0);
            if (err_is_fail(err)) {
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
                return err;
            }

        }
        
        // Update current cluster number to next cluster number
        curr_nr = next_nr;
        
    }
    
    return err;
    
}



// READ AND WRITE FOR DIRENT

errval_t read_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_read) {
    
    errval_t err;
    
    assert(dirent != NULL);
    assert(bytes_read != NULL);

    // Current file size
    size_t file_size;
    
    // Get the current file size from parent directory entry
    err = get_dir_entry_size(dirent->parent_cluster_nr, dirent->parent_pos, &file_size);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Check if requested region start is in file bounds
    if (start >= file_size) {
        *bytes_read = 0;
        return err;
    }
    
    // Check if entire requested region in file bounds and if not shorten it
    if (start + bytes > file_size) {
        bytes = (start + bytes) - file_size;
    }
    
    //assert(start + bytes <= file_size);
    
    // Read cluster chain with appropriate region
    err = read_cluster_chain(dirent->first_cluster_nr, buffer, start, bytes);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Return actual bytes read
    *bytes_read = bytes;
    
    return err;
    
}

errval_t write_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_written) {
    
    errval_t err;
    
    assert(dirent != NULL);
    assert(bytes_written != NULL);
    
    // Current file size
    size_t file_size;
    
    // Get the current file size from parent directory entry
    err = get_dir_entry_size(dirent->parent_cluster_nr, dirent->parent_pos, &file_size);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Current data buffer
    uint8_t *data = buffer;
    
    // Check if requested region start is in file bounds and add padding if required
    if (start > file_size) {
 
        // Size of zeroed out data to be written
        size_t zero_size = start - file_size;
        
        // Allocate data buffer to encapsulate zeros padding in the front
        data = calloc(1, zero_size + bytes);
        
        // Copy over buffer data
        memcpy(data + zero_size, buffer, bytes);
        
        // Update start to include padding
        start = file_size;
        
        // Update bytes to include padding
        bytes += zero_size;
        
    }
    
    // Bytes per cluster
    size_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Old count of clusters (round up)
    size_t old_cluster_count = (file_size + (BytesPerClus - 1)) / BytesPerClus;
    
    // New count of clusters (round up)
    size_t new_cluster_count = (start + bytes + (BytesPerClus - 1)) / BytesPerClus;
    
    // Check if appending of new clusters is required
    if (old_cluster_count < new_cluster_count) {
        
        // Append cluster chain by difference
        append_cluster_chain(dirent->first_cluster_nr, new_cluster_count - old_cluster_count);
    
    }
    
    // Write data to cluster chain region
    err = write_cluster_chain(dirent->first_cluster_nr, data, start, bytes);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Update file size
    file_size = MAX(file_size, start + bytes);

    // Set new file size in parent directory entry
    err = set_dir_entry_size(dirent->parent_cluster_nr, dirent->parent_pos, file_size);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Return actual bytes written (can be more due to padding)
    *bytes_written = bytes;
    
    return err;
    
}

errval_t truncate_dirent(struct fat_dirent *dirent, size_t bytes) {
    
    errval_t err;
    
    err = set_dir_entry_size(dirent->parent_cluster_nr, dirent->parent_pos, bytes);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Bytes per cluster
    size_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Calculate amount of old clusters
    size_t old_cluster_count = (dirent->size + (BytesPerClus - 1)) / BytesPerClus;

    
    // Calculate amount of new clusters needed
    size_t new_cluster_count = (bytes + (BytesPerClus - 1)) / BytesPerClus;
    
    if (old_cluster_count < new_cluster_count) {
    
        err = append_cluster_chain(dirent->first_cluster_nr, new_cluster_count - old_cluster_count);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
    } else if (old_cluster_count > new_cluster_count) {
        
        // Remove all FAT entries from index new_cluster_count (starting with 0)
        remove_fat_entries_from(dirent->first_cluster_nr, new_cluster_count);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
    }
    
    return err;
    
}



/// CLUSTER CHAINS READ AND WRITE

errval_t read_cluster(size_t cluster_nr, void *buffer) {
    
    errval_t err = SYS_ERR_OK;
    
    // Sector number of cluster that occupies the data
    size_t sector_nr = getFirstSectorOfCluster(cluster_nr);
    
    // Temporal buffer
    uint8_t *temp_buffer = buffer;
    
    for (int i = 0; i < BPB_SecPerClus; i++) {
        
        // Read part of cluster and write it to buffer
        err = mmchs_read_block(sector_nr + i, temp_buffer);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Update temporal buffer
        temp_buffer += BPB_BytsPerSec;
        
    }
    
    return err;
    
}

errval_t write_cluster(size_t cluster_nr, void *buffer) {
    
    errval_t err = SYS_ERR_OK;
    
    // Sector number of cluster that occupies the data
    size_t sector_nr = getFirstSectorOfCluster(cluster_nr);
    
    // Temporal buffer
    uint8_t *temp_buffer = buffer;
    
    for (int i = 0; i < BPB_SecPerClus; i++) {
        
        // Write back part of buffer
        err = mmchs_write_block(sector_nr + i, temp_buffer);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Update temporal buffer
        temp_buffer += BPB_BytsPerSec;

    }
    
    return err;
    
}

/// CLUSTERS READ AND WRITE

// We can assume buffer is big enough
errval_t read_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes) {
    
    errval_t err = SYS_ERR_OK;
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Temporal buffer of data to be read from cluster chain
    uint8_t *temp_buf = buffer;
    
    // Index of first cluster that covers requested region
    size_t start_index = start / BytesPerClus;
    
    // Index of last cluster that covers requested region (inclusive)
    size_t end_index = (start + bytes - 1) / BytesPerClus;
    
    // Offset for first cluster that covers requested region
    size_t offset;
    
    // Carryover for last cluster that covers requested region
    size_t carryover;
    
    // Data buffer for the data of one cluster
    uint8_t data[BytesPerClus];
#if PRINT_DEBUG
    debug_printf("start_index: %zu end_index: %zu\n", start_index, end_index);
#endif
    bool isEOF = false;
    
    int cluster_index;
    
    for (cluster_index = 0; !isEOF && cluster_index <= end_index; cluster_index++) {
        
        if (start_index <= cluster_index && cluster_index <= end_index) {
            
            // Set offset if first cluster that covers requested region
            if (cluster_index == start_index) {
                offset = start % BytesPerClus;
            } else {
                offset = 0;
            }
            
            // Set carryover if last cluster that covers requested region
            if (cluster_index == end_index) {
                carryover = BytesPerClus - ((start + bytes) % BytesPerClus);
            } else {
                carryover = 0;
            }
#if PRINT_DEBUG
            debug_printf("offset: %zu carryover: %zu\n", offset, carryover);
#endif
            // Actual bytes that are copied fron data to temp_buf
            size_t copy_size = BytesPerClus - offset - carryover;

            // Read data from cluster[temp_nr] in data section
            err = read_cluster(temp_nr, (void *) data);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
                return err;
            }
            
            //debug_printf("Copy size %zu\n", copy_size);
            
            // Copy data from data (with offset) into temp_buf
            memcpy(temp_buf, data + offset, copy_size);
            
            // Increment temp_buf pointer by the amount of data copied over
            temp_buf += copy_size;
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
#if PRINT_DEBUG
            debug_printf("is EOF\n");
#endif
            isEOF = true;
        }
#if PRINT_DEBUG
        debug_printf("temp_nr: %zu\n", temp_nr);
#endif
    }
    
    if (isEOF && !(cluster_index > end_index)) {
#if PRINT_DEBUG
        debug_printf("Input range was too big for file\n");
        return FAT_ERR_CLUSTER_BOUNDS;
#endif
    } else {
#if PRINT_DEBUG
        debug_printf("Successfully read file with starting cluster %zu in range [%zu,%zu]\n", cluster_nr, start, start + bytes - 1);
#endif
    }
    
    return err;

}

errval_t write_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes) {
    
    errval_t err = SYS_ERR_OK;
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Temporal buffer of data to be read from cluster chain
    uint8_t *temp_buf = buffer;
    
    // Index of first cluster that covers requested region
    size_t start_index = start / BytesPerClus;
    
    // Index of last cluster that covers requested region (inclusive)
    size_t end_index = (start + bytes - 1) / BytesPerClus;
    
    // Offset for first cluster that covers requested region
    size_t offset;
    
    // Carryover for last cluster that covers requested region
    size_t carryover;
    
    // Data buffer for the data of one cluster
    uint8_t *data = calloc(1, BytesPerClus);
    
    bool isEOF = false;

    int cluster_index;
    
    for (cluster_index = 0; !isEOF && cluster_index <= end_index; cluster_index++) {
        
        if (start_index <= cluster_index && cluster_index <= end_index) {
            
            // Set offset if first cluster that covers requested region
            if (cluster_index == start_index) {
                offset = start % BytesPerClus;
            } else {
                offset = 0;
            }
            
            // Set carryover if last cluster that covers requested region
            if (cluster_index == end_index) {
                carryover = BytesPerClus - ((start + bytes) % BytesPerClus);
            } else {
                carryover = 0;
            }
            
            // Actual bytes that are copied fron temp_buf to data
            size_t copy_size = BytesPerClus - offset - carryover;
            
            // Copy previously stored data into data buffer if not entire cluster is overwritten
            if (copy_size < BytesPerClus) {
                
                // Read data from cluster[temp_nr] in data section
                err = read_cluster(temp_nr, (void *) data);
                if (err_is_fail(err)) {
#if PRINT_DEBUG
                    debug_printf("%s\n", err_getstring(err));
#endif
                    return err;
                }
                
            }
#if PRINT_DEBUG
            debug_printf("offset: %zu carryover: %zu copy_size: %zu\n", offset, carryover, copy_size);
#endif
            
            // Copy data from temp_buf into data (with offset)
            memcpy(data + offset, temp_buf, copy_size);
            
            // Write data to cluster[temp_nr] in data section
            err = write_cluster(temp_nr, (void *) data);
            if (err_is_fail(err)) {
                // Free data
                free(data);
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
                return err;
            }
            
            // Increment temp_buf pointer by the amount of data copied over
            temp_buf += copy_size;
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
            isEOF = true;
        }
        
    }
    
    if (isEOF && cluster_index < end_index) {
#if PRINT_DEBUG
        debug_printf("Input range was too big for file\n");
#endif
        return FAT_ERR_CLUSTER_BOUNDS;
        
    } else {
#if PRINT_DEBUG
        debug_printf("Successfully write file with starting cluster %zu in range [%zu,%zu]\n", cluster_nr, start, start + bytes - 1);
#endif
    }
    
    // Free data
    free(data);
    
    return err;
    
}


uint32_t find_free_fat_entry_and_set(size_t start_search_entry, uint32_t value) {
    
    errval_t err;
    
    // Set it search entry to start with at least to 2
    if (start_search_entry < 2) {
        start_search_entry = 2;
    }
    
    // Number of FAT entries that fit into one sector
    size_t FATEntPerSec = BPB_BytsPerSec/sizeof(uint32_t);

    // Buffer for FAT sector
    uint32_t *buffer = calloc(FATEntPerSec, sizeof(uint32_t));
    
    // FAT sector that contains nth entry
    size_t ThisFATSecNum = BPB_ResvdSecCnt + (start_search_entry / FATEntPerSec);
    
    // Read new FAT sector into buffer
    err = mmchs_read_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return -1;
    }
    
    // FAT entry in that sector
    size_t ThisFATEnt = start_search_entry % FATEntPerSec;

    for (int n = start_search_entry; n < FATEntPerSec * FATSz; n++) {
        
        if (n % FATEntPerSec == 0) {
            
            // Update FAT sector that contains nth entry
            ThisFATSecNum = BPB_ResvdSecCnt + (n / FATEntPerSec);
            
            // Read new FAT sector into buffer
            err = mmchs_read_block(ThisFATSecNum, buffer);
            if (err_is_fail(err)) {
                free(buffer);
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
                return -1;
            }
            
        }
        
        // Update FAT entry in that sector
        ThisFATEnt = n % FATEntPerSec;

        if ((n >= 2) && (buffer[ThisFATEnt] == 0)) {
            
            // Change FAT entry to value without MSB
            buffer[ThisFATEnt] = buffer[ThisFATEnt] & 0xF0000000;
            buffer[ThisFATEnt] = buffer[ThisFATEnt] | value;

            // Write new FAT sector with changed free entry back
            err = mmchs_write_block(ThisFATSecNum, buffer);
            if (err_is_fail(err)) {
                free(buffer);
#if PRINT_DEBUG
                debug_printf("%s\n", err_getstring(err));
#endif
                return -1;
            }
            
            free(buffer);
            
            return n;
            
        }
        
    }
    
    free(buffer);
    
    // Couldn't find a free FAT entry
    return -1;
    
}


errval_t append_cluster_chain(size_t cluster_nr, size_t cluster_count) {
    
    errval_t err;
    
    assert(cluster_count > 0);
    
    // Current cluster number
    size_t curr_nr = cluster_nr;
    
    // Next cluster number
    size_t next_nr;
    
    // End of file
    bool isEOF = false;
    
    // Set cluster chain entries in FAT to zero
    while(!isEOF) {
        
        // Set next cluster number to be next cluster number of current cluster number and check if it is EOC
        if (0x0FFFFFF8 <= (next_nr = getFATEntry(curr_nr))) {
            isEOF = true;
        }
        
        // Check that current cluster number is not one of the first two special FAT entries
        if (next_nr < 2) {
#if PRINT_DEBUG
            debug_printf("Current cluster number is in first two special FAT entries\n");
#endif
            break;
        }
        
        // Set current cluster number FAT entry to zero
        err = setFATEntry(curr_nr, 0);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Update current cluster number to next cluster number
        curr_nr = next_nr;
        
    }
    
    // Entry to start searching for the next free FAT entry
    size_t free_entry = 0;
    
    // Append cluster_count clusters to the end of the cluster chain
    for (int i = 0; i < cluster_count; i++) {
        
        // Find a free FAT entry and set it to value 0x0FFFFFFF
        next_nr = find_free_fat_entry_and_set(free_entry, 0x0FFFFFFF);
        if (next_nr == -1) {
#if PRINT_DEBUG
            debug_printf("FAT full! Couldn't find a free entry...\n");
#endif
            return FAT_ERR_FAT_LOOKUP;
        }
        
        // Update entry to start searching for the next free FAT entry
        free_entry = next_nr + 1;
        
        /*
        // Mark this FAT entry as taken
        err = setFATEntry(next_nr, 0x0FFFFFFF);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        */
        
        // Let current FAT entry point to next (previously free) FAT entry
        err = setFATEntry(curr_nr, next_nr);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }
        
        // Update current cluster number to next cluster number
        curr_nr = next_nr;
        
    }
    
    return err;
    
}



errval_t fatfs_serv_opendir(void *st, char *path, struct fat_dirent **ret_dirent) {

#if PRINT_DEBUG
    debug_printf("Opening directory: -%s-\n", path);
#endif
    errval_t err;
    
    assert(ret_dirent != NULL);
    
    struct fatfs_serv_mount *mount = st;
    
    // Allocate memory for potential new dirent
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Resolve path to find dirent
    err = fat_resolve_path(mount->root, path, &dirent);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Differentiate between root directory and normal directory
    if (dirent != NULL) {
        
        // Check that dirent is a directory
        if (!dirent->is_dir) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
#endif
            return FS_ERR_NOTDIR;
        }
        
        // Return directory
        *ret_dirent = dirent;
        
    } else {
        
        // Return root directory
        *ret_dirent = mount->root;
        
    }

    return err;
    
}

errval_t fatfs_serv_readdir(size_t cluster_nr, size_t dir_index, struct fat_dirent **ret_dirent) {
    
    errval_t err;
    
    assert(ret_dirent != NULL);
    
    size_t ret_count = 0;
    
    err = get_dir_entries_count(cluster_nr, &ret_count);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Check if position count is still less than total count of directory entries
    if (dir_index >= ret_count) {
        
        // Set return dirent to NULL if that is not the case
        *ret_dirent = NULL;
        
        // Return an index bounds error
        return FS_ERR_INDEX_BOUNDS;
        
    }
    
    // Counter of taken entries in directory
    size_t count_index = 0;
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Entries per cluster
    size_t EntriesPerClus = BytesPerClus / 32;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Indicates if End Of File is reached
    bool isEOF = false;
    
    // Data buffer for the data of one cluster
    uint8_t *data = calloc(1, BytesPerClus);
    
    // Directory position index relative to cluster
    size_t pos_index;
    
    for (int cluster_index = 0; !isEOF; cluster_index++) {

        // Read data from cluster[temp_nr] in data section
        err = read_cluster(temp_nr , (void *) data);
        if (err_is_fail(err)) {
            // Free data buffer
            free(data);
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        }

        for (pos_index = 0; pos_index < EntriesPerClus; pos_index++) {
            
            uint8_t first_letter = data[pos_index * 32];
            
            if (first_letter == 0x00) {
                // Free entry and no more entries after this one
                break;
            } else if (first_letter == 0xE5) {
                // Free entry but can be smore entries after this one
                
            } else {
                
                // Check if we have the
                if (count_index == dir_index) {
                    
                    struct DIR_Entry *dir_data = calloc(1, sizeof(struct DIR_Entry));
                    
                    // Convert data to directory data
                    data_to_dir_data(dir_data, &data[pos_index * 32]);
#if PRINT_DEBUG
                    debug_printf("%s %zu\n", dir_data->Name, count_index);
#endif
                    /*
                    for (int j = 0; j < 10; j++) {
                        for (int i = 0; i < 32; i+=8) {
                            debug_printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
                                         data[j*32 + i], data[j*32 + i + 1],
                                         data[j*32 + i + 2], data[j*32 + i + 3],
                                         data[j*32 + i + 4], data[j*32 + i + 5],
                                         data[j*32 + i + 6], data[j*32 + i + 7]
                                         );
                        }
                    }
                    */
                    
                    // Create and return dirent from dir_data
                    *ret_dirent = create_dirent(dir_data->Name,
                                                dir_data->FstClus,
                                                dir_data->FileSize,
                                                dir_data->Attr & 0x10,
                                                cluster_nr,
                                                (cluster_index * EntriesPerClus) + pos_index);
                    
                    // Free directory data
                    free(dir_data);
                    
                    // Free data buffer
                    free(data);
                    
                    return err;
                    
                }
                
                // Taken entry
                count_index++;
            }
            
        }
        
        // Update temp_nr to be next cluster_nr and check if it is EOF
        if (0x0FFFFFF8 <= (temp_nr = getFATEntry(temp_nr))) {
            isEOF = true;
        }
        
    }
    
    // Free data buffer
    free(data);
    
    // Return a name could not be found error
    return FS_ERR_NOTFOUND;
    
}

errval_t fatfs_serv_mkdir(void *st, char *path) {
    
    errval_t err;
    
    struct fatfs_serv_mount *mount = st;
    
    // Allocate memory for potential new dirent
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    err = fat_resolve_path(mount->root, path, &dirent);
    if (err_is_ok(err)) {
        // Free  dirent
        free(dirent);
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(FS_ERR_EXISTS));
#endif
        return FS_ERR_EXISTS;
    }
    
    // Free dirent
    free(dirent);
    
   
    struct fat_dirent *parent = calloc(1, sizeof(struct fat_dirent));
    memcpy(parent, mount->root, sizeof(struct fat_dirent));
    char *childname;
    
    // Find parent directory
    char *lastsep = strrchr(path, FS_PATH_SEP);
    
    if (lastsep != NULL) {
        
        childname = lastsep + 1;
        
        size_t pathlen = lastsep - path;
        char pathbuf[pathlen + 1];
        memcpy(pathbuf, path, pathlen);
        pathbuf[pathlen] = '\0';
        
        // Resolve parent directory
        err = fat_resolve_path(mount->root, pathbuf, &parent);
        if (err_is_fail(err)) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(err));
#endif
            return err;
        } else if (parent != NULL && !parent->is_dir) {
#if PRINT_DEBUG
            debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
#endif
            return FS_ERR_NOTDIR; // parent is not a directory
        }
        
        assert(mount->root != NULL);
        
    } else {
        
        childname = path;
        
    }
    
    // Insert into directory
    
    // Search for a free FAT entry and claim it
    size_t dir_cluster_nr = find_free_fat_entry_and_set(0, 0x0FFFFFFF);
    
#if PRINT_DEBUG
    debug_printf("dir_cluster_nr: %zu\n", dir_cluster_nr);
#endif
    
    // Convert name to fat name
    char *fat_name = convert_to_fat_name(childname);
    
    // Construct directory entry in parent directory
    struct DIR_Entry *dir_data = calloc(1, sizeof(struct DIR_Entry));
    
    memcpy(dir_data->Name, fat_name, 12);
    dir_data->Attr = 0x10;
    //dir_data->NTRes = 0;
    dir_data->CrtTimeTenth = 0;
    dir_data->WrtTime = 0;
    dir_data->WrtDate = 0;
    dir_data->FstClus = dir_cluster_nr;
    dir_data->FileSize = 0;
    
    // Free fat name
    free(fat_name);
    
    // First cluster number of parent
    size_t parent_cluster_nr;
    
    // Depending on parent set parents cluster number
    if (parent != NULL) {
        
        // Set parent cluster number as parent directory cluster number
        parent_cluster_nr = parent->first_cluster_nr;
        
        // Free parent dirent
        free(parent);
        
    } else {
        
        assert(mount->root != NULL);
        
        // Set parent cluster number as root directory cluster number
        parent_cluster_nr = mount->root->first_cluster_nr;
        
    }
    
    // Construct "." (dot) entry
    struct DIR_Entry dot_data;
    get_dot_dir_data(dir_cluster_nr, &dot_data);
    
    // Set first entry to dot entry
    set_dir_entry(dir_cluster_nr, 0, &dot_data);
    
    // Construct ".." (dot dot) entry
    struct DIR_Entry dot_dot_data;
    get_dot_dot_dir_data(parent_cluster_nr, &dot_dot_data);
    
    // Set second entry to dot dot entry
    set_dir_entry(dir_cluster_nr, 1, &dot_dot_data);
    
    // Position in parent where entry is inserted
    size_t parent_pos = 0;
    
    // Add directory entry to parent directory
    err = add_dir_entry(parent_cluster_nr, dir_data, &parent_pos);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Free DIR_Entry
    free(dir_data);
    
    return err;
    
}

errval_t fatfs_serv_rmdir(void *st, char *path) {
    
    errval_t err;
    
    struct fatfs_serv_mount *mount = st;
    
    // Allocate dirent
    struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Resolve path to find file dirent
    err = fat_resolve_path(mount->root, path, &dirent);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    if (dirent == NULL) {
        
        // Set dirent to root dirent
        dirent = mount->root;
        
        // Returns not empty error for root directory
        return FS_ERR_NOTEMPTY;
        
    }
    
    // Check if dirent is a directory
    if (!dirent->is_dir) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
#endif
        return FS_ERR_NOTDIR;
    }
    
    // Directory entry count
    size_t dir_count = 0;
    
    // Get total count of directory entries
    err = get_dir_entries_count(dirent->first_cluster_nr, &dir_count);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
#if PRINT_DEBUG
    debug_printf("dir_count: %zu", dir_count);
#endif
    
    // Check if directory has more than 2 ("." and "..") and return a not empty error
    if (dir_count > 2) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(FS_ERR_NOTEMPTY));
#endif
        return FS_ERR_NOTEMPTY;
    }
    
    // Remove the directory entry in parent
    err = remove_dir_entry(dirent->parent_cluster_nr, dirent->parent_pos);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    // Zeros out all FAT entries of dirent's cluster chain
    err = remove_fat_entries(dirent->first_cluster_nr);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
        return err;
    }
    
    return err;
    
}


static void get_dot_dir_data(size_t cluster_nr, struct DIR_Entry *dir_data) {
    
    assert(dir_data != NULL);
    
    char *dot_name = ".          ";
    
    memcpy(dir_data->Name, &dot_name[0], 11);
    dir_data->Name[11] = '\0';
    dir_data->Attr = 0x10;
    //dir_data->NTRes = 0;                // Recommended just to assume it is 0
    dir_data->CrtTimeTenth = 0;
    dir_data->WrtTime = 0;
    dir_data->WrtDate = 0;
    dir_data->FstClus = cluster_nr;
    dir_data->FileSize = 0;
    
}

static void get_dot_dot_dir_data(size_t cluster_nr, struct DIR_Entry *dir_data) {
    
    assert(dir_data != NULL);
    
    char *dot_name = "..         ";
    
    memcpy(dir_data->Name, &dot_name[0], 11);
    dir_data->Name[11] = '\0';
    dir_data->Attr = 0x10;
    //dir_data->NTRes = 0;                // Recommended just to assume it is 0
    dir_data->CrtTimeTenth = 0;
    dir_data->WrtTime = 0;
    dir_data->WrtDate = 0;
    dir_data->FstClus = cluster_nr;
    dir_data->FileSize = 0;
    
}

static void data_to_dir_data(struct DIR_Entry *dest, void *src) {
    
    // Source data buffer
    uint8_t *data = src;
    
    // Destination directory data struct
    struct DIR_Entry *dir_data = dest;
    
    // Construct directory data from data buffer
    memcpy(dir_data->Name, &data[0], 11);
    dir_data->Name[11] = '\0';
    dir_data->Attr = data[11];
    //dir_data->NTRes = 0;                // Recommended just to assume it is 0
    dir_data->CrtTimeTenth = data[13];
    memcpy(&dir_data->WrtTime, &data[22], 2);
    memcpy(&dir_data->WrtDate, &data[24], 2);
    memcpy(&dir_data->FstClus, &data[26], 2);                       // Low bits
    memcpy(((uint8_t *) &dir_data->FstClus) + 2, &data[20], 2);     // High bits
    memcpy(&dir_data->FileSize, &data[28], 4);
    
}

static void dir_data_to_data(void *dest, struct DIR_Entry *src) {
    
    // Source directory data struct
    struct DIR_Entry *dir_data = src;
    
    // Destination data buffers
    uint8_t *data = dest;
    
    // Construct data buffer from directory data
    memcpy(&data[0], dir_data->Name, 11);
    data[11] = dir_data->Attr;
    //data[12] = 0;                      // Recommended just to assume it is 0
    data[13] = dir_data->CrtTimeTenth;
    
    memcpy(&data[20], ((uint8_t *) &dir_data->FstClus) + 2, 2);     // High bits
    memcpy(&data[22], &dir_data->WrtTime, 2);
    memcpy(&data[24], &dir_data->WrtDate, 2);
    memcpy(&data[26], &dir_data->FstClus, 2);                       // Low bits
    memcpy(&data[28], &dir_data->FileSize, 2);
    
}

errval_t update_dirent_size(struct fat_dirent *dirent) {
    
    errval_t err;
    
    // Return size of dirent
    size_t ret_size = 0;
    
    // Get dirent size
    err = get_dir_entry_size(dirent->parent_cluster_nr, dirent->parent_pos, &ret_size);
    if (err_is_fail(err)) {
#if PRINT_DEBUG
        debug_printf("%s\n", err_getstring(err));
#endif
    }
    
    // Update dirent size
    dirent->size = ret_size;
    
    return err;
    
}


