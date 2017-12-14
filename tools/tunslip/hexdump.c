#include <stdio.h>
#include <ctype.h>
 
#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 8
#endif


 
void hexdump(void *mem, unsigned int len)
{
        char * mem_ch = (char *)mem;
        unsigned int i, j;
        
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        fprintf(stderr, "0x%06x: ", i);
                }
 
                /* print hex data */
                if(i < len)
                {
                        fprintf(stderr, "%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        fprintf(stderr, "   ");
                }
                
                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        fprintf(stderr, " ");
                                }
                                else if(isprint((int) mem_ch[j]))
                                {
                                        fprintf(stderr, "%c", 0xFF & ((char*)mem)[j]);        
                                }
                                else /* other char */
                                {
                                        fprintf(stderr, ".");
                                }
                        }
                        fprintf(stderr,"\n");
                }
        }
}
