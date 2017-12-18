//
//  fat_helper.h
//  DoritOS
//

#ifndef fat_helper_h
#define fat_helper_h

#define FS_EXTENSION_SEP '.'

char *convert_to_fat_name(const char *name);

char *convert_to_normal_name(char *fat_name);

#endif /* fat_helper_h */
