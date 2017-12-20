//
//  fs_fat.h
//  DoritOS
//

#ifndef fs_fat_h
#define fs_fat_h

struct fat_dirent {
    
    size_t size;
    
    char name[12];                     // string null terminated
    
    size_t parent_cluster_nr;       // 0 if root directory
    size_t parent_pos;              // 0 if root directory
    
    bool is_dir;
    
    size_t first_cluster_nr;
    
};


#endif /* fs_fat_h */
