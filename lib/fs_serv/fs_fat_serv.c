//
//  fs_fat_serv.c
//  DoritOS
//

#include <stdio.h>
#include <string.h>
#include <aos/aos.h>

#include <fs_serv/fs_fat_serv.h>

extern errval_t mmchs_read_block(size_t block_nr, void *buffer);
extern errval_t mmchs_write_block(size_t block_nr, void *buffer);


/// INITIALIZATION

static uint16_t BPB_BytsPerSec;
static uint8_t BPB_SecPerClus;
static uint16_t BPB_ResvdSecCnt;
static uint8_t BPB_NumFATs;
static uint16_t BPB_RootEntCnt;
static uint16_t BPB_TotSec16;
static uint8_t BPB_Media;
static uint16_t BPB_FATSz16;
static uint16_t BPB_SecPerTrk;
static uint16_t BPB_NumHeads;
static uint32_t BPB_HiddSec;
static uint32_t BPB_TotSec32;
static uint32_t BPB_FATSz32;
static uint16_t BPB_ExtFlags;
static uint16_t BPB_FSVer;
static uint32_t BPB_RootClus;
static uint16_t BPB_FSInfo;
static uint16_t BPB_BkBootSec;
static uint8_t BPB_Reserved[12];
static uint8_t BS_DrvNum;
static uint8_t BS_Reserved1;
static uint8_t BS_BootSig;
static uint32_t BS_VolID;
static char BS_VolLab[12];
static char BS_FilSysType[9];

// Calculated
static uint32_t FirstDataSector;
static uint32_t RootDirSectors;
static uint32_t FATSz;

#define FS_PATH_SEP '/'

errval_t init_BPB(void) {
    
    // Boot Sector and BPB Structure
    
    errval_t err;
    
    uint8_t data[512];
    
    err = mmchs_read_block(0, data);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    //struct boot_sector *bs = buf;
    
    BPB_BytsPerSec = GET_BYTES2(data, 11);
    debug_printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
    
    BPB_SecPerClus = GET_BYTES1(data, 13);
    debug_printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
    
    BPB_ResvdSecCnt = GET_BYTES2(data, 14);
    debug_printf("BPB_ResvdSecCnt: %d\n", BPB_ResvdSecCnt);
    
    BPB_NumFATs = GET_BYTES1(data, 16);
    debug_printf("BPB_NumFATs: %d\n", BPB_NumFATs);
    
    BPB_RootEntCnt = GET_BYTES2(data, 17);
    debug_printf("BPB_RootEntCnt: %d\n", BPB_RootEntCnt);
    
    BPB_TotSec16 = GET_BYTES2(data, 19);
    debug_printf("BPB_TotSec16: %d\n", BPB_TotSec16);
    
    BPB_Media = GET_BYTES1(data, 21);
    debug_printf("BPB_Media: %x\n", BPB_Media);
    
    BPB_FATSz16 = GET_BYTES2(data, 22);
    debug_printf("BPB_FATSz16: %d\n", BPB_FATSz16);
    
    BPB_SecPerTrk = GET_BYTES2(data, 24);
    debug_printf("BPB_SecPerTrk: %d\n", BPB_SecPerTrk);
    
    BPB_NumHeads = GET_BYTES2(data, 26);
    debug_printf("BPB_NumHeads: %d\n", BPB_NumHeads);
    
    BPB_HiddSec = GET_BYTES4(data, 28);
    debug_printf("BPB_HiddSec: %d\n", BPB_HiddSec);
    
    BPB_TotSec32 = GET_BYTES4(data, 32);
    debug_printf("BPB_TotSec32: %d\n", BPB_TotSec32);
    
    BPB_FATSz32 = GET_BYTES4(data, 36);
    debug_printf("BPB_FATSz32: %d\n", BPB_FATSz32);
    
    BPB_ExtFlags = GET_BYTES2(data, 40);
    debug_printf("BPB_ExtFlags: %x\n", BPB_ExtFlags);
    
    BPB_FSVer = GET_BYTES2(data, 42);
    debug_printf("BPB_FSVer: %x\n", BPB_FSVer);
    
    BPB_RootClus = GET_BYTES4(data, 44);
    debug_printf("BPB_RootClus: %d\n", BPB_RootClus);
    
    BPB_FSInfo = GET_BYTES2(data, 48);
    debug_printf("BPB_FSInfo: %d\n", BPB_FSInfo);
    
    BPB_BkBootSec = GET_BYTES2(data, 50);
    debug_printf("BPB_BkBootSec: %d\n", BPB_BkBootSec);
    
    memcpy(BPB_Reserved, &data[52], 12);
    debug_printf("BPB_Reserved: ");
    for (int i = 0; i < 12; ++i) {
        printf("%02x ", BPB_Reserved[i]);
    }
    printf("\n");
    
    BS_DrvNum = GET_BYTES1(data, 64);
    debug_printf("BS_DrvNum: %d\n", BS_DrvNum);
    
    BS_Reserved1 = GET_BYTES1(data, 65);
    debug_printf("BS_Reserved1: %d\n", BS_Reserved1);
    
    BS_BootSig = GET_BYTES1(data, 66);
    debug_printf("BS_BootSig: %d\n", BS_BootSig);
    
    BS_VolID = GET_BYTES4(data, 67);
    debug_printf("BS_VolID: %x\n", BS_VolID);
    
    memcpy(BS_VolLab, &data[71], 11);
    BS_VolLab[11] = '\0';
    debug_printf("BS_VolLab: %s\n", BS_VolLab);
    
    memcpy(BS_FilSysType, &data[82], 8);
    BS_FilSysType[8] = '\0';
    debug_printf("BS_FilSysType: %s\n", BS_FilSysType);
    
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
    
    // Buffer for FAT sector
    uint32_t buffer[FATEntPerSec];
    
    // Read FAT sector
    err = mmchs_read_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Return entry in FAT with MSB masked out
    return buffer[ThisFATEnt] & 0x0FFFFFFF;
    
}


__attribute__((unused))
errval_t setFATEntry(size_t n, uint32_t value) {
 
    errval_t err;
 
    // Number of FAT entries that fit into one sector
    size_t FATEntPerSec = BPB_BytsPerSec/sizeof(uint32_t);

    // FAT sector that contains nth entry
    size_t ThisFATSecNum = BPB_ResvdSecCnt + (n / FATEntPerSec);
 
    // FAT entry in that sector
    size_t ThisFATEnt = n % FATEntPerSec;

    // Buffer for FAT sector
    uint32_t buffer[FATEntPerSec];
 
    // Change FAT entry in FAT_buffer
    buffer[ThisFATEnt] = buffer[ThisFATEnt] & 0xF0000000;
    buffer[ThisFATEnt] = buffer[ThisFATEnt] | value;
 
    // Read FAT sector of SD card
    err = mmchs_read_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
 
    // Change FAT entry to value without MSB
    buffer[ThisFATEnt] = buffer[ThisFATEnt] & 0xF0000000;
    buffer[ThisFATEnt] = buffer[ThisFATEnt] | value;
 
    // Write FAT sector back to SD card
    err = mmchs_write_block(ThisFATSecNum, buffer);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
 
    return err;
 
}


/* LEGACY
/// FILETABLE

static struct filetab_entry filetab[MAX_FILE];

int filetab_alloc(struct filetab_entry *new_entry)
{
    for (int i = MIN_FILE; i < MAX_FILE; i++) {
        
        if (filetab[i].type == FILETAB_TYPE_AVAILABLE) {
            memcpy(&filetab[i], new_entry, sizeof(struct filetab_entry));
            
            return i;
        }
        
    }
    
    // filetable full
    //errno = EMFILE;
    return -1;
}

struct filetab_entry *filetab_get(int index)
{
    static struct filetab_entry invalid = {
        .type = FILETAB_TYPE_AVAILABLE,
        .refcount = 0,
        .dirent = NULL
    };
    
    if (index < MIN_FILE || index >= MAX_FILE) {
        debug_printf("INVALID\n");
        return &invalid;
    } else {
        debug_printf("VALID\n");
        return &filetab[index];
    }
}

void filetab_free(int index)
{
    assert(index >= MIN_FILE && index < MAX_FILE);
    assert(filetab[index].type != FILETAB_TYPE_AVAILABLE);

    filetab[index].type = FILETAB_TYPE_AVAILABLE;
    filetab[index].refcount = 0;
    filetab[index].dirent = NULL;
    
}

static int filetab_check_dirent(struct fat_dirent *dirent) {
    
    assert(dirent != NULL);
    
    for (int i = MIN_FILE; i < MAX_FILE; i++) {
    
        if (filetab[i].type != FILETAB_TYPE_AVAILABLE) {
            
            assert(filetab[i].dirent != NULL);
            
            if (filetab[i].dirent->first_cluster_nr == dirent->first_cluster_nr) {
                
                debug_printf("This file is already in filetab!\n");
                
                return i;
                
            }
            
        }
        
    }
    
    return -1;
    
}
*/


// FIXME: Add flags
errval_t fat_open(void *st, char *path, struct fat_dirent **ret_dirent) {
    
    debug_printf("Opening file: -%s-\n", path);
    
    errval_t err;
    
    assert(ret_dirent != NULL);
    
    struct fat32_mount *mount = st;
    
    // Allocate memory for potential new file dirent
    struct fat_dirent *file_dirent = calloc(1, sizeof(struct fat_dirent));
    
    // Resolve path to find file dirent
    err = fat_resolve_path(mount->root, path, &file_dirent);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Set return dirent
    *ret_dirent = file_dirent;
    
    return err;
    
}

// FIXME: Add flags
errval_t fat_resolve_path(struct fat_dirent *root, char *path, struct fat_dirent **ret_dirent) {
    
    errval_t err = SYS_ERR_OK;
    
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
    struct fat_dirent *next_dirent;
    
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
        
        // Find directory entry with same name as in pathbuf
        err = fat_find_dirent(curr_dirent, pathbuf, &next_dirent);
        if (err_is_fail(err)) {
            // Free both current and next dirent
            free(curr_dirent);
            free(next_dirent);
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        // Check if last level and not a file
        if (!next_dirent->is_dir && nextsep != NULL) {
            // Free both current and next dirent
            free(curr_dirent);
            free(next_dirent);
            debug_printf("%s\n", err_getstring(FS_ERR_NOTDIR));
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

// FIXME: char *name can be null terminated, but has to have the same first 11 bytes
errval_t fat_find_dirent(struct fat_dirent *curr_dirent, char *name, struct fat_dirent **ret_dirent) {
    
    errval_t err;

    assert(curr_dirent != NULL);
    assert(ret_dirent != NULL);
    
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
            // Free data buffer
            free(data);
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        for (int pos_index = 0; pos_index < BytesPerClus / 32; pos_index++) {
            
            uint8_t *dirent_data = &data[pos_index * 32];
            
            if (memcmp(name, &dirent_data[0], 11) == 0) {
                
                debug_printf("SAME NAME\n");
                
                // Allocating memory for return dirent
                struct fat_dirent *dirent = calloc(1, sizeof(struct fat_dirent));
                
                // Set refcount of dirent to 1
                dirent->refcount = 1;
                
                // Set size of dirent
                dirent->size = *((uint32_t *) &dirent_data[28]);
                
                // Set name of dirent
                memcpy(&dirent->name, &dirent_data[0], 11);
                
                // Set parent directory first cluster number
                dirent->parent_cluster_nr = curr_dirent->first_cluster_nr;
                
                // Set position in parent directory
                dirent->parent_pos = (cluster_index * BytesPerClus / 32) + pos_index;
                
                // Set if dirent is a directory
                dirent->is_dir = dirent_data[11] & 0x10;
                
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
    
    // Free data buffer
    free(data);
    
    return FAT_ERR_FAT_LOOKUP;
    
}


// Inclusive . and ..
static size_t getDirEntriesCount(size_t cluster_nr) {
    
    errval_t err;
    
    // Counter of taken entries in directory
    size_t count = 0;
    
    // Bytes per cluster
    uint32_t BytesPerClus = BPB_BytsPerSec * BPB_SecPerClus;
    
    // Temporal cluster number
    size_t temp_nr = cluster_nr;
    
    // Indicates if End Of File is reached
    bool isEOF = false;
    
    // Data buffer for the data of one cluster
    uint8_t data[BytesPerClus];
    
    for (int cluster_index = 0; !isEOF; cluster_index++) {
        
        // Read data from cluster[temp_nr] in data section
        err = read_cluster(temp_nr , (void *) data);
        if (err_is_fail(err)) {
            debug_printf("%s\n", err_getstring(err));
            return err;
        }
        
        for (int pos_index = 0; pos_index < BytesPerClus / 32; pos_index++) {
            
            uint8_t dirent_data = data[pos_index * 32];
            
            if (dirent_data == 0x00) {
                // Free entry and no more entries after this one
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

    debug_printf("Successfully looked through directory file with starting cluster %zu\n", cluster_nr);
    
    debug_printf("Directory has %zu entries\n", count);
    
    return err;
    
}


//void openDir()

/* UNUSED
static struct DIR_Entry getDIREntry(size_t cluster_nr, size_t pos) {
    
    errval_t err;
    
    uint8_t data[32];
    
    err = read_cluster_chain(cluster_nr, (void *) data, pos * 32, 32);
    if (err_is_fail(err)) {
        debug_printf("Error with getting DIR_Entry: %s\n", err_getstring(err));
        // return NULL;
    }
    
    struct DIR_Entry ret_entry;
    
    memcpy(&ret_entry.Name, &data[0], 11);
    ret_entry.Name[11] = '\0';
    
    ret_entry.Attr = data[11];
    ret_entry.NTRes = 0;                // Recommended just to assume it is 0
    ret_entry.CrtTimeTenth = data[13];
    
    memcpy(&ret_entry.WrtTime, &data[22], 2);
    memcpy(&ret_entry.WrtDate, &data[24], 2);
    memcpy(&ret_entry.FstClus, &data[26], 2);                       // Low bits
    memcpy(((uint8_t *) &ret_entry.FstClus) + 2, &data[20], 2);     // High bits
    memcpy(&ret_entry.FileSize, &data[28], 4);
    
    //ret_entry.WrtTime = *((uint16_t *) &data[22]);
    //ret_entry.WrtDate = *((uint16_t *) &data[24]);
    //ret_entry.FstClus = *((uint16_t *) &data[26]) | *((uint16_t *) &data[20]) << 16;
    //ret_entry.FileSize = *((uint32_t *) &data[28]);
    
    return ret_entry;
    
}

static void setDIREntry(size_t cluster_nr, size_t pos, struct DIR_Entry entry) {
    
    errval_t err;
    
    uint8_t data[32];
    
    err = read_cluster_chain(cluster_nr, (void *) data, pos * 32, 32);
    if (err_is_fail(err)) {
        debug_printf("Error with setting DIR_Entry: %s\n", err_getstring(err));
        return;
    }
    
    memcpy(&data[0], &entry.Name, 11);
    
    data[11] = entry.Attr;
    data[12] = 0;                      // Recommended just to assume it is 0
    data[13] = entry.CrtTimeTenth;
    
    // *((uint16_t *) &data[20]) = (entry.FstClus >> 16) & 0xFFFF;     // High bits
    // *((uint16_t *) &data[22]) = entry.WrtTime;
    // *((uint16_t *) &data[24]) = entry.WrtDate;
    // *((uint16_t *) &data[26]) = entry.FstClus & 0xFFFF;             // Low bits
    // *((uint32_t *) &data[28]) = entry.FileSize;
 
    memcpy(&data[20], ((uint8_t *) &entry.FstClus) + 2, 2);
    memcpy(&data[22], &entry.WrtTime, 2);
    memcpy(&data[24], &entry.WrtDate, 2);
    memcpy(&data[26], &entry.FstClus, 2);
    memcpy(&data[28], &entry.FileSize, 2);
 
    err = write_cluster_chain(cluster_nr, (void *) data, pos * 32, 32);
    if (err_is_fail(err)) {
        debug_printf("Error with setting DIR_Entry: %s\n", err);
        return;
    }
    
}
*/


/// FAT_DIRENT

static struct fat_dirent *create_dirent(char *name, size_t first_cluster_nr, size_t size, bool is_dir,
                                          size_t parent_cluster_nr, size_t parent_pos) {
    
    struct fat_dirent *ret_dirent = calloc(1, sizeof(struct fat_dirent));
    
    ret_dirent->refcount = 0;
    
    ret_dirent->size = size;
    
    memcpy(&ret_dirent->name, name, 12);
    
    ret_dirent->parent_cluster_nr = parent_cluster_nr;
    
    ret_dirent->parent_pos = parent_pos;
    
    ret_dirent->is_dir = is_dir;
    
    ret_dirent->first_cluster_nr = first_cluster_nr;
    
    return ret_dirent;
    
}

/* UNUSED
static void delete_dirent(struct fat_dirent *dirent) {
    
    assert(dirent != NULL);
    assert(dirent->refcount == 0);
    
    dirent->refcount = 0;
    
    dirent->size = 0;
    
    if (dirent->name != NULL) {
        free(dirent->name);
        dirent->name = NULL;
    }
    
    dirent->parent_cluster_nr = 0;;
    
    dirent->parent_pos = 0;
    
    dirent->is_dir = false;
    
    dirent->first_cluster_nr = 0;
    
    //free(dirent);
    
    dirent = NULL;
    
}
*/
/*
static void open_dirent(struct fat_dirent *dirent) {
    
    assert(dirent != NULL);
    
    dirent->refcount += 1;
    
}
*/
/* UNUSED
static void close_dirent(struct fat_dirent *dirent) {
    
    assert(dirent != NULL);

    dirent->refcount -= 1;
    
}
*/

errval_t init_root_dir(void *st) {
    
    struct fat32_mount *mount = st;
    
    mount->root = create_dirent("/", BPB_RootClus, 0, true, 0, 0);
    
    //open_dirent(mount->root);
    
    size_t count = getDirEntriesCount(mount->root->first_cluster_nr);
    
    debug_printf("Amount of entries in the root directory: %z\n", count);
    
    return SYS_ERR_OK;
    
}

// ADD AND REMOVE FOR DIRENT

//errval_t add_dirent(struct fat_dirent *dirent) { }

errval_t remove_dirent(struct fat_dirent *dirent) {
    
    errval_t err;
    
    debug_printf("Not yet implemented FAT zeroing\n");
    
    assert(dirent != NULL);
    
    // Remove the directory entry in parent
    err = remove_entry(dirent->parent_cluster_nr, dirent->parent_cluster_nr);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    /*
    size_t cluster_index;
    
    // Set cluster chain entries in FAT to zero
    for (cluster_index = 0; !isEOF && cluster_index <= end_index; cluster_index++) {
        
        
        
    }
    */
    
    return LIB_ERR_NOT_IMPLEMENTED;
    
}



errval_t remove_entry(size_t cluster_nr, size_t pos) {
    
    errval_t err;
    
    // Set directory entry region [0, 10] to indicate free entry
    uint8_t empty_name_buffer[11];
    empty_name_buffer[0] = 0xEF;
    
    err = write_cluster_chain(cluster_nr, empty_name_buffer, pos * 32, 11);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    // Set directory entry region [20, 31] zero
    uint8_t empty_rest_buffer[12];
    
    err = write_cluster_chain(cluster_nr, empty_rest_buffer, pos * 32 + 20, 12);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
    }
    
    return err;
    
}


// READ AND WRITE FOR DIRENT

errval_t read_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_read) {
    
    errval_t err;
    
    assert(dirent != NULL);
    
    assert(start + bytes <= dirent->size);
    
    err = read_cluster_chain(dirent->first_cluster_nr, buffer, start, bytes);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    assert(bytes_read != NULL);
    
    // FIXME: Proper implementation
    *bytes_read = bytes;
    
    return err;
    
}

errval_t write_dirent(struct fat_dirent *dirent, void *buffer, size_t start, size_t bytes, size_t *bytes_written) {
    
    errval_t err;
    
    assert(dirent != NULL);
    
    assert(start + bytes <= dirent->size);
    
    err = write_cluster_chain(dirent->first_cluster_nr, buffer, start, bytes);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    assert(bytes_written != NULL);
    
    // FIXME: Proper implementation
    *bytes_written = bytes;
    
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
            debug_printf("%s\n", err_getstring(err));
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
            debug_printf("%s\n", err_getstring(err));
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
    
    debug_printf("start_index: %zu end_index: %zu\n", start_index, end_index);
    
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
            
            debug_printf("offset: %zu carryover: %zu\n", offset, carryover);
            
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
            debug_printf("is EOF\n");
            isEOF = true;
        }
        
        debug_printf("temp_nr: %zu\n", temp_nr);
        
    }
    
    if (isEOF && !(cluster_index > end_index)) {
        
        debug_printf("Input range was too big for file\n");
        return FAT_ERR_CLUSTER_BOUNDS;
        
    } else {
        
        debug_printf("Successfully read file with starting cluster %zu in range [%zu,%zu]\n", cluster_nr, start, start + bytes);
        
    }
    
    return err;

}

errval_t write_cluster_chain(size_t cluster_nr, void *buffer, size_t start, size_t bytes) {
    
    // TODO: LOCK!!!
    
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
                    debug_printf("%s\n", err_getstring(err));
                    return err;
                }
                
            }
            
            // Copy data from temp_buf into data (with offset)
            memcpy(data + offset, temp_buf, copy_size);
            
            // Write data to cluster[temp_nr] in data section
            err = write_cluster(temp_nr, (void *) data);
            if (err_is_fail(err)) {
                debug_printf("%s\n", err_getstring(err));
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
    
    if (isEOF && !(cluster_index > end_index)) {
        
        debug_printf("Input range was too big for file\n");
        return FAT_ERR_CLUSTER_BOUNDS;
        
    } else {
        
        debug_printf("Successfully write file with starting cluster %zu in range [%zu,%zu]", cluster_nr, start, start + bytes);
        
    }
    
    // TODO: UNLOCK!!!

    return err;
    
}







