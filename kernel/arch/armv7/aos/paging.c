/*
 * Copyright (c) 2009-2013,2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <kernel.h>
#include <dispatch.h>
#include <cache.h>
#include <cp15.h>
#include <paging_kernel_arch.h>
#include <string.h>
#include <exceptions.h>
#include <platform.h>
#include <cap_predicates.h>
#include <dispatch.h>
#include <mdb/mdb_tree.h>

#define MSG(format, ...) printk( LOG_NOTE, "ARMv7-A: "format, ## __VA_ARGS__ )

inline static uintptr_t paging_round_down(uintptr_t address, uintptr_t size)
{
    return address & ~(size - 1);
}

inline static uintptr_t paging_round_up(uintptr_t address, uintptr_t size)
{
    return (address + size - 1) & ~(size - 1);
}

inline static int aligned(uintptr_t address, uintptr_t bytes)
{
    return (address & (bytes - 1)) == 0;
}

static void map_kernel_section_hi(lvaddr_t va, union arm_l1_entry l1);
static union arm_l1_entry make_dev_section(lpaddr_t pa);
static void paging_print_l1_pte(lvaddr_t va, union arm_l1_entry pte);

void paging_print_l1(void);

/* In the non-boot paging code, these are pointers to be set to the values
 * passed from the boot driver. */
union arm_l1_entry *l1_low;
union arm_l1_entry *l1_high;
union arm_l2_entry *l2_vec;

void paging_load_pointers(struct arm_core_data *boot_core_data) {
    l1_low= (union arm_l1_entry *)
        local_phys_to_mem(boot_core_data->kernel_l1_low);
    l1_high= (union arm_l1_entry *)
        local_phys_to_mem(boot_core_data->kernel_l1_high);
    l2_vec= (union arm_l2_entry *)
        local_phys_to_mem(boot_core_data->kernel_l2_vec);
}

static void map_kernel_section_hi(lvaddr_t va, union arm_l1_entry l1)
{
    assert( va >= MEMORY_OFFSET );
    l1_high[ARM_L1_OFFSET(va)] = l1;
}

/**
 * /brief Return an L1 page table entry to map a 1MB 'section' of
 * device memory located at physical address 'pa'.
 */
static union arm_l1_entry make_dev_section(lpaddr_t pa)
{
    union arm_l1_entry l1;

    l1.raw = 0;
    l1.section.type = L1_TYPE_SECTION_ENTRY;
    // l1.section.tex       = 1;
    l1.section.bufferable   = 0;
    l1.section.cacheable    = 0;
    l1.section.ap10         = 3; // prev value: 3 // RW/NA RW/RW
    // l1.section.ap10         = 1;    // RW/NA
    l1.section.ap2          = 0;
    l1.section.base_address = ARM_L1_SECTION_NUMBER(pa);
    return l1;
}

/**
 * \brief Return whether we have enabled the MMU. Useful for
 * initialization assertions
 */
bool paging_mmu_enabled(void)
{
    return true;
}

/* Map the exception vectors at VECTORS_BASE. */
void
paging_map_vectors(void) {
    /* The addresses installed into the page tables must be physical. */
    lpaddr_t vectors_phys= mem_to_local_phys((lvaddr_t)exception_vectors);
    lpaddr_t l2_vec_phys=  mem_to_local_phys((lvaddr_t)l2_vec);

    MSG("Mapping vectors at P:%"PRIxLPADDR" to %"PRIxLVADDR
        " using L2 table at P:%"PRIxLPADDR"\n",
        vectors_phys, VECTORS_BASE, l2_vec_phys);

    /**
     * Install a single small page mapping to cover the vectors.
     *
     * The mapping fields are set exactly as for the kernel's RAM sections -
     * see make_ram_section() for details.
     */
    union arm_l2_entry *e_l2= &l2_vec[ARM_L2_OFFSET(VECTORS_BASE)];
    e_l2->small_page.type= L2_TYPE_SMALL_PAGE;
    e_l2->small_page.tex=        1;
    e_l2->small_page.cacheable=  1;
    e_l2->small_page.bufferable= 1;
    e_l2->small_page.not_global= 0;
    e_l2->small_page.shareable=  1;
    e_l2->small_page.ap10=       1;
    e_l2->small_page.ap2=        0;

    /* The vectors must be at the beginning of a frame. */
    assert((vectors_phys & BASE_PAGE_MASK) == 0);
    e_l2->small_page.base_address= vectors_phys >> BASE_PAGE_BITS;

    /* Clean the modified entry to L2 cache. */
    clean_to_pou(e_l2);

    /**
     * Map the L2 table to hold the high vectors mapping.
     */
    union arm_l1_entry *e_l1= &l1_high[ARM_L1_OFFSET(VECTORS_BASE)];
    e_l1->page_table.type= L1_TYPE_PAGE_TABLE_ENTRY;
    e_l1->page_table.base_address= l2_vec_phys >> ARM_L2_TABLE_BITS;

    /* Clean the modified entry to L2 cache. */
    clean_to_pou(e_l1);

    /* We shouldn't need to invalidate any TLB entries, as this entry has
     * never been mapped. */
}

/**
 * \brief Map a device into the kernel's address space.  
 * 
 * \param device_base is the physical address of the device
 * \param device_size is the number of bytes of physical address space
 * the device occupies. 
 *
 * \return the kernel virtual address of the mapped device, or panic. 
 */
lvaddr_t paging_map_device(lpaddr_t dev_base, size_t dev_size)
{
    // We map all hardware devices in the kernel using sections in the
    // top quarter (0xC0000000-0xFE000000) of the address space, just
    // below the exception vectors.  
    // 
    // It makes sense to use sections since (1) we don't map many
    // devices in the CPU driver anyway, and (2) if we did, it might
    // save a wee bit of TLB space. 
    //

    // First, we make sure that the device fits into a single
    // section. 
    if (ARM_L1_SECTION_NUMBER(dev_base) != ARM_L1_SECTION_NUMBER(dev_base+dev_size-1)) {
        panic("Attempt to map device spanning >1 section 0x%"PRIxLPADDR"+0x%x\n",
              dev_base, dev_size );
    }
    
    // Now, walk down the page table looking for either (a) an

    // existing mapping, in which case return the address the device
    // is already mapped to, or an invalid mapping, in which case map
    // it. 
    uint32_t dev_section = ARM_L1_SECTION_NUMBER(dev_base);
    uint32_t dev_offset  = ARM_L1_SECTION_OFFSET(dev_base);
    lvaddr_t dev_virt    = 0;
    
    for( size_t i = ARM_L1_OFFSET( DEVICE_OFFSET - 1); i > ARM_L1_MAX_ENTRIES / 4 * 3; i-- ) {

        // Work out the virtual address we're looking at
        dev_virt = (lvaddr_t)(i << ARM_L1_SECTION_BITS);

        // If we already have a mapping for that address, return it. 
        if ( L1_TYPE(l1_high[i].raw) == L1_TYPE_SECTION_ENTRY &&
             l1_high[i].section.base_address == dev_section ) {
            return dev_virt + dev_offset;
        }

        // Otherwise, if it's free, map it. 
        if ( L1_TYPE(l1_high[i].raw) == L1_TYPE_INVALID_ENTRY ) {
            map_kernel_section_hi(dev_virt, make_dev_section(dev_base));
            invalidate_data_caches_pouu(true);
            invalidate_tlb(); /* XXX selective */
            return dev_virt + dev_offset;
        } 
    }
    // We're all out of section entries :-(
    panic("Ran out of section entries to map a kernel device");
}

/**
 * \brief Print out a L1 page table entry 'pte', interpreted relative
 * to a given virtual address 'va'. 
 */
static void paging_print_l1_pte(lvaddr_t va, union arm_l1_entry pte)
{
    printf("(memory offset=%x):\n", va);
    if ( L1_TYPE(pte.raw) == L1_TYPE_INVALID_ENTRY) {
        return;
    }
    printf( " %x-%"PRIxLVADDR": ", va, va + ARM_L1_SECTION_BYTES - 1);
    switch( L1_TYPE(pte.raw) ) { 
    case L1_TYPE_INVALID_ENTRY:
        printf("INVALID\n");
        break;
    case L1_TYPE_PAGE_TABLE_ENTRY:
        printf("L2 PT 0x%"PRIxLPADDR" pxn=%d ns=%d sbz=%d dom=0x%04x sbz1=%d \n", 
               pte.page_table.base_address << 10, 
               pte.page_table.pxn,
               pte.page_table.ns,
               pte.page_table.sbz0,
               pte.page_table.domain,
               pte.page_table.sbz1 );
        break;
    case L1_TYPE_SECTION_ENTRY:
        printf("SECTION 0x%"PRIxLPADDR" buf=%d cache=%d xn=%d dom=0x%04x\n", 
               pte.section.base_address << 20, 
               pte.section.bufferable,
               pte.section.cacheable,
               pte.section.execute_never,
               pte.section.domain );
        printf("      sbz0=%d ap=0x%03x tex=0x%03x shr=%d ng=%d mbz0=%d ns=%d\n",
               pte.section.sbz0,
               (pte.section.ap2) << 2 | pte.section.ap10,
               pte.section.tex,
               pte.section.shareable,
               pte.section.not_global,
               pte.section.mbz0,
               pte.section.ns );
        break;
    case L1_TYPE_SUPER_SECTION_ENTRY:
        printf("SUPERSECTION 0x%"PRIxLPADDR" buf=%d cache=%d xn=%d dom=0x%04x\n", 
               pte.super_section.base_address << 24, 
               pte.super_section.bufferable,
               pte.super_section.cacheable,
               pte.super_section.execute_never,
               pte.super_section.domain );
        printf("      sbz0=%d ap=0x%03x tex=0x%03x shr=%d ng=%d mbz0=%d ns=%d\n",
               pte.super_section.sbz0,
               (pte.super_section.ap2) << 2 | pte.super_section.ap10,
               pte.super_section.tex,
               pte.super_section.shareable,
               pte.super_section.not_global,
               pte.super_section.mbz0,
               pte.super_section.ns );
        break;
    }
}

/**
 * /brief Print out the CPU driver's two static page tables.  Note:
 * 
 * 1) This is a lot of output.  Each table has 4096 entries, each of
 *    which takes one or two lines of output.
 * 2) The first half of the TTBR1 table is similarly used, and is
 *    probably (hopefully) all empty. 
 * 3) The second half of the TTBR0 table is similarly never used, and
 *    hopefully empty. 
 * 4) The TTBR0 table is only used anyway at boot, since thereafter it
 *    is replaced by a user page table. 
 * Otherwise, go ahead and knock yourself out. 
 */
void paging_print_l1(void)
{
    size_t i;
    lvaddr_t base = 0;
    printf("TTBR1 table:\n");
    for(i = 0; i < ARM_L1_MAX_ENTRIES; i++, base += ARM_L1_SECTION_BYTES ) { 
        paging_print_l1_pte(base, l1_high[i]);
    }
    printf("TTBR0 table:\n");
    base = 0;
    for(i = 0; i < ARM_L1_MAX_ENTRIES; i++, base += ARM_L1_SECTION_BYTES ) { 
        paging_print_l1_pte(base, l1_low[i]);
    }
}


size_t do_unmap(lvaddr_t pt, cslot_t slot, size_t num_pages)
{
    size_t unmapped_pages = 0;
    union arm_l2_entry *ptentry = (union arm_l2_entry *)pt + slot;
    for (int i = 0; i < num_pages; i++) {
        ptentry++->raw = 0;
        unmapped_pages++;
    }
    return unmapped_pages;
}

void paging_dump_tables(struct dcb *dispatcher)
{
    if (!local_phys_is_valid(dispatcher->vspace)) {
        printk(LOG_ERR, "dispatcher->vspace = 0x%"PRIxLPADDR": too high!\n" ,
               dispatcher->vspace);
        return;
    }
    lvaddr_t l1 = local_phys_to_mem(dispatcher->vspace);

    for (int l1_index = 0; l1_index < ARM_L1_MAX_ENTRIES; l1_index++) {
        // get level2 table
        union arm_l1_entry *l1_e = (union arm_l1_entry *)l1 + l1_index;
        if (!l1_e->raw) { continue; }
        if (l1_e->invalid.type == 2) { // section 
            genpaddr_t paddr = (genpaddr_t)(l1_e->section.base_address) << 20;
            printf("%d: (section) 0x%"PRIxGENPADDR"\n", l1_index, paddr);
        } else if (l1_e->invalid.type == 1) { // l2 table
            genpaddr_t ptable_gp = (genpaddr_t)(l1_e->page_table.base_address) << 10;
            lvaddr_t ptable_lv = local_phys_to_mem(gen_phys_to_local_phys(ptable_gp));

            printf("%d: (l2table) 0x%"PRIxGENPADDR"\n", l1_index, ptable_gp);

            for (int entry = 0; entry < ARM_L2_MAX_ENTRIES; entry++) {
                union arm_l2_entry *e =
                    (union arm_l2_entry *)ptable_lv + entry;
                genpaddr_t paddr = (genpaddr_t)(e->small_page.base_address) << BASE_PAGE_BITS;
                if (!paddr) {
                    continue;
                }
                printf("%d.%d: 0x%"PRIxGENPADDR"\n", l1_index, entry, paddr);
            }
        }
    }
}

/**
 * /brief Install a level 2 page table entry located at l2e, to map
 * physical address 'pa', with flags 'flags'.   'flags' here is in the
 * form of a prototype 32-bit L2 *invalid* PTE with address 0.
 */
void paging_set_l2_entry(uintptr_t* l2e, lpaddr_t addr, uintptr_t flags)
{
    union arm_l2_entry e;
    e.raw = flags;

    assert( L2_TYPE(e.raw) == L2_TYPE_INVALID_PAGE );
    assert( e.small_page.base_address == 0);
    assert( ARM_PAGE_OFFSET(addr) == 0 );

    e.small_page.type = L2_TYPE_SMALL_PAGE;
    e.small_page.base_address = (addr >> 12);

    *l2e = e.raw;

    /* Clean the modified entry to L2 cache. */
    clean_to_pou(l2e);
}
