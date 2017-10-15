#include <aos/aos.h>
#include <spawn/spawn.h>

#include <elf/elf.h>
#include <aos/dispatcher_arch.h>
#include <barrelfish_kpi/paging_arm_v7.h>
#include <barrelfish_kpi/domain_params.h>
#include <spawn/multiboot.h>

extern struct bootinfo *bi;


// Set up the cspace for a child process
static errval_t spawn_setup_cspace(struct spawninfo *si) {
    
    // TODO: Reduce caprefs on stack!!!
    
    errval_t err;
    
    // Create a L1 cnode
    struct capref root_cnode_cap;
    struct cnoderef root_cnode_ref;
    err = cnode_create_l1(&root_cnode_cap, &root_cnode_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Create L2 cnode: TASKCN
    struct cnoderef taskcn_ref;
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_TASKCN, &taskcn_ref);
    if (err_is_fail(err)) {
        return err;
    }

    //  Create SLOT_DISPATCHER capability
    struct capref slot_dispatcher_cap = {
        .cnode = taskcn_ref,
        .slot = TASKCN_SLOT_DISPATCHER
    };
    err = dispatcher_create(slot_dispatcher_cap);
    if (err_is_fail(err)) {
        return err;
    }
    
    //  Retype SLOT_DISPATCHER capability into SLOT_SELFEP
    struct capref slot_selfep_cap = {
        .cnode = taskcn_ref,
        .slot = TASKCN_SLOT_SELFEP
    };
    err = cap_retype(slot_selfep_cap, slot_dispatcher_cap, 0, ObjType_EndPoint, 0, 1);
    if (err_is_fail(err)) {
        return err;
    }
    
    //  Copy root cnode capability into SLOT_ROOTCN
    struct capref slot_rootcn_cap = {
        .cnode = taskcn_ref,
        .slot = TASKCN_SLOT_ROOTCN
    };
    cap_copy(slot_rootcn_cap, root_cnode_cap);
    
    
    //  TODO: SLOT_DISPFRAME and SLOT_ARGSPG
    
    
    // Create L2 cnode: SLOT_ALLOC0
    struct cnoderef slot_alloc0_ref;
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_SLOT_ALLOC0, &slot_alloc0_ref);
    if (err_is_fail(err)) {
        return err;
    }

    
    // Create L2 cnode: SLOT_ALLOC1
    struct cnoderef slot_alloc1_ref;
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_SLOT_ALLOC1, &slot_alloc1_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    
    // Create L2 cnode: SLOT_ALLOC2
    struct cnoderef slot_alloc2_ref;
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_SLOT_ALLOC2, &slot_alloc2_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    
    // Create L2 cnode: SLOT_BASE_PAGE_CN
    struct cnoderef slot_base_page_ref;
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_BASE_PAGE_CN, &slot_base_page_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    //  Create RAM capabilities for SLOT_BASE_PAGE_CN
    struct capref ram_cap;
    err = ram_alloc(&ram_cap, BASE_PAGE_SIZE * L2_CNODE_SLOTS);
    if (err_is_fail(err)) {
        return err;
    }
    struct capref base_page_ram_cap = {
        .cnode = slot_base_page_ref,
        .slot = 0
    };
    err = cap_retype(base_page_ram_cap, ram_cap, 0, ObjType_RAM, BASE_PAGE_SIZE, L2_CNODE_SLOTS);
    if (err_is_fail(err)) {
        return err;
    }

    // Create L2 cnode: SLOT_PAGECN
    err = cnode_create_foreign_l2(root_cnode_cap, ROOTCN_SLOT_PAGECN, &si->slot_pagecn_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    return SYS_ERR_OK;
    
}

// Set up the vspace for a child process
static errval_t spawn_setup_vspace(struct spawninfo *si) {
    
    errval_t err = SYS_ERR_OK;
    
    // Creating L1 VNode
    struct capref l1_pt_cap = {
        .cnode = si->slot_pagecn_ref,
        .slot = PAGECN_SLOT_VROOT
    };
    err = vnode_create(l1_pt_cap, ObjType_VNode_ARM_l1);
    if (err_is_fail(err)) {
        return err;
    }
    
    return err;
    
}

// TODO(M2): Implement this function such that it starts a new process
// TODO(M4): Build and pass a messaging channel to your child process
errval_t spawn_load_by_name(void * binary_name, struct spawninfo * si) {
    printf("spawn start_child: starting: %s\n", binary_name);

    errval_t err = SYS_ERR_OK;

    // Init spawninfo
    memset(si, 0, sizeof(*si));
    si->binary_name = binary_name;

    // TODO: Implement me
    // - Get the binary from multiboot image
    // - Map multiboot module in your address space
    // - Setup childs cspace
    // - Setup childs vspace
    // - Load the ELF binary
    // - Setup dispatcher
    // - Setup environment
    // - Make dispatcher runnable

    // Finding the memory region containing the ELF image
    struct mem_region *mem = multiboot_find_module(bi, si->binary_name);
    if (!mem) {
        return SPAWN_ERR_FIND_MODULE;
    }

    // Constructing the capability for the frame containing the ELF image
    struct capref child_frame = {
        .cnode = cnode_module,
        .slot = mem->mrmod_slot
    };

    // Mapping the ELF image into the virtual address space
    void *buf = NULL;
    paging_map_frame_attr(get_current_paging_state(), &buf, mem->mrmod_size, child_frame, VREGION_FLAGS_READ, NULL, NULL);

    char *elf = buf;
    debug_printf("Mapped ELF into memory: 0x%x %c%c%c\n", elf[0], elf[1], elf[2], elf[3]);
    
    // Set up cspace
    err = spawn_setup_cspace(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up cspace: %s\n", err_getstring(err));
        return err;
    }
    
    // Set up cspace
    err = spawn_setup_vspace(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up vspace: %s\n", err_getstring(err));
        return err;
    }

    return err;
}
