//
//  fat_helper.h
//  DoritOS
//

#ifndef fat_helper_h
#define fat_helper_h

/// FAT CONSTANTS

uint16_t BPB_BytsPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_ResvdSecCnt;
uint8_t BPB_NumFATs;
uint16_t BPB_RootEntCnt;
uint16_t BPB_TotSec16;
uint8_t BPB_Media;
uint16_t BPB_FATSz16;
uint16_t BPB_SecPerTrk;
uint16_t BPB_NumHeads;
uint32_t BPB_HiddSec;
uint32_t BPB_TotSec32;
uint32_t BPB_FATSz32;
uint16_t BPB_ExtFlags;
uint16_t BPB_FSVer;
uint32_t BPB_RootClus;
uint16_t BPB_FSInfo;
uint16_t BPB_BkBootSec;
uint8_t BPB_Reserved[12];
uint8_t BS_DrvNum;
uint8_t BS_Reserved1;
uint8_t BS_BootSig;
uint32_t BS_VolID;
char BS_VolLab[12];
char BS_FilSysType[9];

// Calculated
uint32_t FirstDataSector;
uint32_t RootDirSectors;
uint32_t FATSz;

#define FS_PATH_SEP '/'

#define FS_EXTENSION_SEP '.'

char *convert_to_fat_name(const char *name);

char *convert_to_normal_name(char *fat_name);

#endif /* fat_helper_h */
