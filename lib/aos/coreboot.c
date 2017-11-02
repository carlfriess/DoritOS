/*
 * Copyright (c) 2016, ETH Zurich.
 *
 * This file is distributed under the terms in the attached LICENSE file.  If
 * you do not find this file, copies can be found by writing to: ETH Zurich
 * D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>

#include <aos/aos.h>
#include <aos/coreboot.h>
#include <elf/elf.h>

extern struct bootinfo *bi;

errval_t
load_cpu_relocatable_segment(void *elfdata, void *out, lvaddr_t vbase,
                             lvaddr_t text_base, lvaddr_t *got_base) {
    /* Find the full loadable segment, as it contains the dynamic table. */
    struct Elf32_Phdr *phdr_full= elf32_find_segment_type(elfdata, PT_LOAD);
    if(!phdr_full) return ELF_ERR_HEADER;
    void *full_segment_data= elfdata + phdr_full->p_offset;

    printf("Loadable segment at V:%08"PRIx32"\n", phdr_full->p_vaddr);

    /* Find the relocatable segment to load. */
    struct Elf32_Phdr *phdr= elf32_find_segment_type(elfdata, PT_BF_RELOC);
    if(!phdr) return ELF_ERR_HEADER;

    printf("Relocatable segment at V:%08"PRIx32"\n", phdr->p_vaddr);

    /* Copy the raw segment data. */
    void *in= elfdata + phdr->p_offset;
    assert(phdr->p_filesz <= phdr->p_memsz);
    memcpy(out, in, phdr->p_filesz);

    /* Find the dynamic segment. */
    struct Elf32_Phdr *phdr_dyn= elf32_find_segment_type(elfdata, PT_DYNAMIC);
    if(!phdr_dyn) return ELF_ERR_HEADER;

    printf("Dynamic segment at V:%08"PRIx32"\n", phdr_dyn->p_vaddr);

    /* The location of the dynamic segment is specified by its *virtual
     * address* (vaddr), relative to the loadable segment, and *not* by its
     * p_offset, as for every other segment. */
    struct Elf32_Dyn *dyn=
        full_segment_data + (phdr_dyn->p_vaddr - phdr_full->p_vaddr);

    /* There is no *entsize field for dynamic entries. */
    size_t n_dyn= phdr_dyn->p_filesz / sizeof(struct Elf32_Dyn);

    /* Find the relocations (REL only). */
    void *rel_base= NULL;
    size_t relsz= 0, relent= 0;
    void *dynsym_base= NULL;
    const char *dynstr_base= NULL;
    size_t syment= 0, strsz= 0;
    for(size_t i= 0; i < n_dyn; i++) {
        switch(dyn[i].d_tag) {
            /* There shouldn't be any RELA relocations. */
            case DT_RELA:
                return ELF_ERR_HEADER;

            case DT_REL:
                if(rel_base != NULL) return ELF_ERR_HEADER;

                /* Pointers in the DYNAMIC table are *virtual* addresses,
                 * relative to the vaddr base of the segment that contains
                 * them. */
                rel_base= full_segment_data +
                    (dyn[i].d_un.d_ptr - phdr_full->p_vaddr);
                break;

            case DT_RELSZ:
                relsz= dyn[i].d_un.d_val;
                break;

            case DT_RELENT:
                relent= dyn[i].d_un.d_val;
                break;

            case DT_SYMTAB:
                dynsym_base= full_segment_data +
                    (dyn[i].d_un.d_ptr - phdr_full->p_vaddr);
                break;

            case DT_SYMENT:
                syment= dyn[i].d_un.d_val;
                break;

            case DT_STRTAB:
                dynstr_base= full_segment_data +
                    (dyn[i].d_un.d_ptr - phdr_full->p_vaddr);
                break;

            case DT_STRSZ:
                strsz= dyn[i].d_un.d_val;
        }
    }
    if(rel_base == NULL || relsz == 0 || relent == 0 ||
       dynsym_base == NULL || syment == 0 ||
       dynstr_base == NULL || strsz == 0)
        return ELF_ERR_HEADER;

    /* XXX - The dynamic segment doesn't actually tell us the size of the
     * dynamic symbol table, which is very annoying.  We should fix this by
     * defining and implementing a standard format for dynamic executables on
     * Barrelfish, using DT_PLTGOT.  Currently, GNU ld refuses to generate
     * that for the CPU driver binary. */
    assert((size_t)dynstr_base > (size_t)dynsym_base);
    size_t dynsym_len= (size_t)dynstr_base - (size_t)dynsym_base;

    /* Walk the symbol table to find got_base. */
    size_t dynsym_offset= 0;
    struct Elf32_Sym *got_sym= NULL;
    while(dynsym_offset < dynsym_len) {
        got_sym= dynsym_base + dynsym_offset;
        if(!strcmp(dynstr_base + got_sym->st_name, "got_base")) break;

        dynsym_offset+= syment;
    }
    if(dynsym_offset >= dynsym_len) {
        printf("got_base not found.\n");
        return ELF_ERR_HEADER;
    }

    /* Addresses in the relocatable segment are relocated to the
     * newly-allocated region, relative to their addresses in the relocatable
     * segment.  Addresses outside the relocatable segment are relocated to
     * the shared text segment, relative to their position in the
     * originally-loaded segment. */
    uint32_t relocatable_offset= vbase - phdr->p_vaddr;
    uint32_t text_offset= text_base - phdr_full->p_vaddr;

    /* Relocate the got_base within the relocatable segment. */
    *got_base= vbase + (got_sym->st_value - phdr->p_vaddr);

    /* Process the relocations. */
    size_t n_rel= relsz / relent;
    printf("Have %zu relocations of size %zu\n", n_rel, relent);
    for(size_t i= 0; i < n_rel; i++) {
        struct Elf32_Rel *rel= rel_base + i * relent;

        size_t sym=  ELF32_R_SYM(rel->r_info);
        size_t type= ELF32_R_TYPE(rel->r_info);

        /* We should only see relative relocations (R_ARM_RELATIVE) against
         * sections (symbol 0). */
        if(sym != 0 || type != R_ARM_RELATIVE) return ELF_ERR_HEADER;

        uint32_t offset_in_seg= rel->r_offset - phdr->p_vaddr;
        uint32_t *value= out + offset_in_seg;

        uint32_t offset;
        if(*value >= phdr->p_vaddr &&
           (*value - phdr->p_vaddr) < phdr->p_memsz) {
            /* We have a relocation to an address *inside* the relocatable
             * segment. */
            offset= relocatable_offset;
            //printf("r ");
        }
        else {
            /* We have a relocation to an address in the shared text segment.
             * */
            offset= text_offset;
            //printf("t ");
        }

        //printf("REL@%08"PRIx32" %08"PRIx32" -> %08"PRIx32"\n",
               //rel->r_offset, *value, *value + offset);
        *value+= offset;
    }

    return SYS_ERR_OK;
}
