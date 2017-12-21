#include <aos/aos.h>
#include <spawn/spawn.h>

#include <elf/elf.h>
#include <aos/dispatcher_arch.h>
#include <barrelfish_kpi/paging_arm_v7.h>
#include <barrelfish_kpi/domain_params.h>
#include <spawn/multiboot.h>
#include <aos/lmp.h>
#include <aos/waitset.h>

extern struct bootinfo *bi;


static void add_parent_mapping(struct spawninfo *si, void *addr) {
    // Check if parent_mappings exists
    struct parent_mapping *mapping = (struct parent_mapping *) malloc(sizeof(struct parent_mapping));
    if (si->parent_mappings == NULL) {
        mapping->addr = addr;
        mapping->next = NULL;
        si->parent_mappings = mapping;
    } else {
        // Iterate to last entry, allocate new mapping, assign
        struct parent_mapping *i;
        for (i = si->parent_mappings; i->next != NULL; i = i->next);
        mapping->addr = addr;
        mapping->next = NULL;
        i->next = mapping;
    }
}

// Set up the cspace for a child process
static errval_t spawn_setup_cspace(struct spawninfo *si) {
    
    errval_t err;

    // Placeholder cnoderef/capref to reduce stack size
    struct cnoderef cnoderef_l1;

    struct capref capref_alpha;
    struct capref capref_beta;

    // Create a L1 cnode
    err = cnode_create_l1(&si->child_rootcn_cap, &cnoderef_l1);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Create L2 cnode: TASKCN
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_TASKCN, &si->taskcn_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    //  Copy the IRQ capability
    capref_alpha.cnode = si->taskcn_ref;
    capref_alpha.slot = TASKCN_SLOT_IRQ;
    capref_beta.cnode = cnode_task;
    capref_beta.slot = TASKCN_SLOT_IRQ;
    err = cap_copy(capref_alpha, capref_beta);
    if (err_is_fail(err)) {
        return err;
    }


    //  Create SLOT_DISPATCHER capability
    capref_alpha.cnode = si->taskcn_ref;
    capref_alpha.slot = TASKCN_SLOT_DISPATCHER;

    err = dispatcher_create(capref_alpha);
    if (err_is_fail(err)) {
        return err;
    }

    //  Copy the SLOT_DISPATCHER capability to parent cspace
    slot_alloc(&si->child_dispatcher_cap);
    err = cap_copy(si->child_dispatcher_cap, capref_alpha);
    if (err_is_fail(err)) {
        return err;
    }

    //  Retype SLOT_DISPATCHER capability into SLOT_SELFEP
    capref_beta.cnode = si->taskcn_ref;
    capref_beta.slot = TASKCN_SLOT_SELFEP;

    err = cap_retype(capref_beta, capref_alpha, 0, ObjType_EndPoint, 0, 1);
    if (err_is_fail(err)) {
        return err;
    }

    //  Copy root cnode capability into SLOT_ROOTCN
    si->slot_rootcn_cap.cnode = si->taskcn_ref;
    si->slot_rootcn_cap.slot = TASKCN_SLOT_ROOTCN;
    err = cap_copy(si->slot_rootcn_cap, si->child_rootcn_cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    // Create L2 cnode: SLOT_ALLOC0
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_SLOT_ALLOC0, &si->slot_alloc0_ref);
    if (err_is_fail(err)) {
        return err;
    }

    // Create L2 cnode: SLOT_ALLOC1
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_SLOT_ALLOC1, NULL);
    if (err_is_fail(err)) {
        return err;
    }


    // Create L2 cnode: SLOT_ALLOC2
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_SLOT_ALLOC2, NULL);
    if (err_is_fail(err)) {
        return err;
    }


    // Create L2 cnode: SLOT_BASE_PAGE_CN
    struct cnoderef cnoderef_base_pagecn;
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_BASE_PAGE_CN, &cnoderef_base_pagecn);
    if (err_is_fail(err)) {
        return err;
    }

    //  Create RAM capabilities for SLOT_BASE_PAGE_CN
    err = ram_alloc(&capref_alpha, BASE_PAGE_SIZE * L2_CNODE_SLOTS);
    if (err_is_fail(err)) {
        return err;
    }

    capref_beta.cnode = cnoderef_base_pagecn;
    capref_beta.slot = 0;

    err = cap_retype(capref_beta, capref_alpha, 0, ObjType_RAM, BASE_PAGE_SIZE, L2_CNODE_SLOTS);
    if (err_is_fail(err)) {
        return err;
    }

    cap_delete(capref_alpha);
    slot_free(capref_alpha);

    // Create L2 cnode: SLOT_PAGECN
    err = cnode_create_foreign_l2(si->child_rootcn_cap, ROOTCN_SLOT_PAGECN, &si->slot_pagecn_ref);
    if (err_is_fail(err)) {
        return err;
    }
    
    return SYS_ERR_OK;
    
}

// Set up the lmp channel
static errval_t spawn_setup_lmp_channel(struct spawninfo *si) {
    errval_t err = SYS_ERR_OK;

    // Allocate and initialize lmp channel
    si->pi->lc = (struct lmp_chan *) malloc(sizeof(struct lmp_chan));

    // Open channel to messages
    err = lmp_chan_accept(si->pi->lc, LMP_RECV_LENGTH, NULL_CAP);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    //  Create INITEP
    struct capref initep = {
        .cnode = si->taskcn_ref,
        .slot = TASKCN_SLOT_INITEP
    };

    // Copy lmp channel endpoint into child
    err = cap_copy(initep, si->pi->lc->local_cap);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    // Allocate new recv slot for lmp channel
    err = lmp_chan_alloc_recv_slot(si->pi->lc);
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }
    
    // Register callback handler
    struct waitset *default_ws = get_default_waitset();
    err = lmp_chan_register_recv(si->pi->lc, default_ws, MKCLOSURE(lmp_server_dispatcher, (void *) si->pi->lc));
    if (err_is_fail(err)) {
        debug_printf("%s\n", err_getstring(err));
        return err;
    }

    return err;
}

// Set up the vspace for a child process
static errval_t spawn_setup_vspace(struct spawninfo *si) {
    
    errval_t err = SYS_ERR_OK;
    
    // Allocate a frame for the child's paging state
    struct capref paging_state_frame_cap;
    size_t paging_state_frame_size = sizeof(struct paging_state);
    err = frame_alloc(&paging_state_frame_cap, paging_state_frame_size, &paging_state_frame_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Allocate two frames for the child's paging state slab allocators
    struct capref slab_frame_1_cap;
    struct capref slab_frame_2_cap;
    size_t slab_frame_1_size = BASE_PAGE_SIZE;
    size_t slab_frame_2_size = BASE_PAGE_SIZE;
    err = frame_alloc(&slab_frame_1_cap, slab_frame_1_size, &slab_frame_1_size);
    if (err_is_fail(err)) {
        return err;
    }
    err = frame_alloc(&slab_frame_2_cap, slab_frame_2_size, &slab_frame_2_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map the frames into parent's virtual address space
    err = paging_map_frame(get_current_paging_state(), (void **) &si->child_paging_state, paging_state_frame_size, paging_state_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    void *slab_frame_1_addr;
    err = paging_map_frame(get_current_paging_state(), (void **) &slab_frame_1_addr, slab_frame_1_size, slab_frame_1_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    void *slab_frame_2_addr;
    err = paging_map_frame(get_current_paging_state(), (void **) &slab_frame_2_addr, slab_frame_2_size, slab_frame_2_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Unmap these frames later PF
    add_parent_mapping(si, si->child_paging_state);
    add_parent_mapping(si, slab_frame_1_addr);
    add_parent_mapping(si, slab_frame_2_addr);
    
    
    // Creating L1 VNode
    si->l1_pt_cap.cnode = si->slot_pagecn_ref;
    si->l1_pt_cap.slot = PAGECN_SLOT_VROOT;
    err = vnode_create(si->l1_pt_cap, ObjType_VNode_ARM_l1);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Copy capability to child's L1 pagetable into parent's cspace
    slot_alloc(&si->child_root_pt_cap);
    cap_copy(si->child_root_pt_cap, si->l1_pt_cap);
    
    // Initialize the child paging state
    err = paging_init_state(si->child_paging_state,
                            MAX(MAX((lvaddr_t) slab_frame_1_addr + slab_frame_1_size, (lvaddr_t) slab_frame_2_addr + slab_frame_2_size), VADDR_OFFSET + paging_state_frame_size),
                            si->child_root_pt_cap,
                            get_default_slot_allocator());
    if (err_is_fail(err)) {
        return err;
    }
    
    // Add memory to slab allocators
    slab_grow(&si->child_paging_state->vspace_slabs, slab_frame_1_addr, slab_frame_1_size);
    slab_grow(&si->child_paging_state->slabs, slab_frame_2_addr, slab_frame_2_size);
    
    // Map the slab allocator frames into the child's virtual memory
    paging_alloc_fixed(si->child_paging_state, slab_frame_1_addr, slab_frame_1_size);
    err = paging_map_fixed(si->child_paging_state, (lvaddr_t) slab_frame_1_addr, slab_frame_1_cap, slab_frame_1_size);
    if (err_is_fail(err)) {
        return err;
    }
    paging_alloc_fixed(si->child_paging_state, slab_frame_2_addr, slab_frame_2_size);
    err = paging_map_fixed(si->child_paging_state, (lvaddr_t) slab_frame_2_addr, slab_frame_2_cap, slab_frame_2_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map the paging state into the child's memory at VADDR_OFFSET
    paging_alloc_fixed(si->child_paging_state, (void *) VADDR_OFFSET, paging_state_frame_size);
    err = paging_map_fixed(si->child_paging_state, VADDR_OFFSET, paging_state_frame_cap, paging_state_frame_size);
    if (err_is_fail(err)) {
        return err;
    }

    return err;
    
}

static errval_t elf_allocator_callback(void *state, genvaddr_t base, size_t size, uint32_t flags, void **ret) {
    
    errval_t err = SYS_ERR_OK;
    struct spawninfo *si = (struct spawninfo *) state;

    // Aligning to page boundry in child's virtual memory
    genvaddr_t real_base = base / BASE_PAGE_SIZE;
    real_base *= BASE_PAGE_SIZE;
    size_t offset = base - real_base;

    // Allocating memory for section in ELF
    struct capref frame_cap;
    size_t ret_size;
    err = frame_alloc(&frame_cap, size + offset, &ret_size);
    if (err_is_fail(err)) {
        return err;
    }

    // Map the memory region into parent virtual address space
    err = paging_map_frame_attr(get_current_paging_state(), ret, ret_size, frame_cap, VREGION_FLAGS_READ_WRITE, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Add mapping to parent mappings list PF
    add_parent_mapping(si, *ret);

    // Adding offset to target address
    *ret += offset;

    // Map the memory region into child's virtual address space.
    uint32_t real_base_addr = real_base;
    paging_alloc_fixed(si->child_paging_state, (void *) real_base_addr, ret_size);
    err = paging_map_fixed_attr(si->child_paging_state, real_base, frame_cap, ret_size, flags);
    if (err_is_fail(err)) {
        return err;
    }
    
    return err;
    
}

// Parse the ELF and copy the sections into memory
static errval_t spawn_parse_elf(struct spawninfo *si, void *elf, size_t elf_size) {
    
    errval_t err = SYS_ERR_OK;

    // Load the ELF
    err = elf_load(EM_ARM, &elf_allocator_callback, (void *) si, (lvaddr_t) elf, elf_size, &si->entry_addr);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Get the address of the .got section
    uint32_t elf_addr = (uint32_t) elf;
    struct Elf32_Shdr *got_header = elf32_find_section_header_name((genvaddr_t) elf_addr, elf_size, ".got");
    si->got_addr = (void *) got_header->sh_addr;
        
    return err;
    
}

// Set up the dispatcher
static errval_t spawn_setup_dispatcher(struct spawninfo *si) {
    
    errval_t err = SYS_ERR_OK;
    
    struct capref dcb_frame_cap;
    size_t dcb_size = 1 << DISPATCHER_FRAME_BITS;
    err = frame_alloc(&dcb_frame_cap, dcb_size, &dcb_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map the memory into parent's virtual address space
    err = paging_map_frame_attr(get_current_paging_state(), &si->dcb_addr_parent, dcb_size, dcb_frame_cap, VREGION_FLAGS_READ_WRITE, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Add mapping to parent mappings list PF
    add_parent_mapping(si, si->dcb_addr_parent);
    
    // Map the memory region into child's virtual address space.
    void *dcb_addr_child;
    err = paging_map_frame_attr(si->child_paging_state, &dcb_addr_child, dcb_size, dcb_frame_cap, VREGION_FLAGS_READ_WRITE, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Copy dcb frame capability into SLOT_DISPFRAME
    si->slot_dispframe_cap.cnode = si->taskcn_ref;
    si->slot_dispframe_cap.slot = TASKCN_SLOT_DISPFRAME;
    cap_copy(si->slot_dispframe_cap, dcb_frame_cap);

    // Get references to dispatcher structs
    dispatcher_handle_t dcb_addr_parent_handle = (dispatcher_handle_t) si->dcb_addr_parent;
    struct dispatcher_shared_generic *disp = get_dispatcher_shared_generic(dcb_addr_parent_handle);
    struct dispatcher_generic *disp_gen = get_dispatcher_generic(dcb_addr_parent_handle);
    struct dispatcher_shared_arm *disp_arm = get_dispatcher_shared_arm(dcb_addr_parent_handle);
    arch_registers_state_t *enabled_area = dispatcher_get_enabled_save_area(dcb_addr_parent_handle);
    arch_registers_state_t *disabled_area = dispatcher_get_disabled_save_area(dcb_addr_parent_handle);
    
    // Set dispatcher information
    disp_gen->core_id = disp_get_core_id();     // Core id of the dispatcher
    disp->udisp = (lvaddr_t) dcb_addr_child;    // Address of dispatcher frame in child's vspace
    disp->disabled = 1;     // Start the dispatcher in disabled mode
    disp->fpu_trap = 1;     // Trap on fpr instructions
    strncpy(disp->name, si->binary_name, DISP_NAME_LEN);    // Name of dispatcher
    
    // Set address of first instruction
    disabled_area->named.pc = si->entry_addr;
    
    // Initialize offest register
    disp_arm->got_base = (lvaddr_t) si->got_addr;
    enabled_area->regs[REG_OFFSET(PIC_REGISTER)] = (lvaddr_t) si->got_addr;
    disabled_area->regs[REG_OFFSET(PIC_REGISTER)] = (lvaddr_t) si->got_addr;

    enabled_area->named.cpsr = CPSR_F_MASK | ARM_MODE_USR;
    disabled_area->named.cpsr = CPSR_F_MASK | ARM_MODE_USR;
    disp_gen->eh_frame = 0;
    disp_gen->eh_frame_size = 0;
    disp_gen->eh_frame_hdr = 0;
    disp_gen->eh_frame_hdr_size = 0;
    
    return err;
    
}

static errval_t spawn_setup_args(struct spawninfo *si, const char *argstring) {

    errval_t err = SYS_ERR_OK;
    
    struct capref argspace_frame_cap;
    size_t size = BASE_PAGE_SIZE;
    err = frame_alloc(&argspace_frame_cap, size, &size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map the memory into parent's virtual address space
    void *argspace_addr_parent;
    err = paging_map_frame_attr(get_current_paging_state(), &argspace_addr_parent, size, argspace_frame_cap, VREGION_FLAGS_READ_WRITE, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }

    // Add mapping to parent mappings list PF
    add_parent_mapping(si, argspace_addr_parent);
    
    // Map the memory region into child's virtual address space.
    void *argspace_addr_child;
    err = paging_map_frame_attr(si->child_paging_state, &argspace_addr_child, size, argspace_frame_cap, VREGION_FLAGS_READ_WRITE, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Copy argspace frame capability into SLOT_ARGSPAGE
    struct capref slot_argspace_cap = {
        .cnode = si->taskcn_ref,
        .slot = TASKCN_SLOT_ARGSPAGE
    };
    cap_copy(slot_argspace_cap, argspace_frame_cap);
    
    // Zero out the argspace
    memset(argspace_addr_parent, 0, BASE_PAGE_SIZE);
    
    // Get a reference to the spawn_domain_params struct
    struct spawn_domain_params *params = (struct spawn_domain_params *) argspace_addr_parent;
    
    void *argspace_offset = argspace_addr_parent + sizeof(struct spawn_domain_params);
    
    // Get length of argstring
    size_t len = strlen(argstring);
    
    // Allocate args
    char *args = (char *) argspace_offset;
    argspace_offset += len;

    // Initialization
    params->argc = 0;
    params->argv[params->argc++] = argspace_addr_child + sizeof(struct spawn_domain_params);


    // Copy argstring to args
    uint8_t escaped = 0;
    char quote = '\0';
    size_t j = 0;
    for (size_t i = 0; i < len && params->argc < MAX_CMDLINE_ARGS; i++) {
        // If escaped, copy and reset escape flag
        if (escaped) {
            args[j++] = argstring[i];
            escaped = !escaped;
        } else {
            // If quote is null
            if (quote == '\0') {
                // Check if quote or space or escape, else copy
                if (argstring[i] == '"' || argstring[i] == '\'') {
                    quote = argstring[i];
                } else if(argstring[i] == ' ') {
                    args[j++] = '\0';
                    params->argv[params->argc++] = params->argv[0] + j;
                } else if (argstring[i] == '\\') {
                    escaped = !escaped;
                } else {
                    args[j++] = argstring[i];
                }
            } else {
                // Check if matching quote, else copy
                if (argstring[i] == quote) {
                    quote = '\0';
                } else {
                    args[j++] = argstring[i];
                }
            }
        }
    }

    // Terminate argv
    params->argv[params->argc] = NULL;
    
    // Terminate envp
    params->envp[0] = NULL;
    
    // Set argument address in register r0 in enabled state of child
    dispatcher_handle_t dcb_addr_parent_handle = (dispatcher_handle_t) si->dcb_addr_parent;
    arch_registers_state_t *enabled_area = dispatcher_get_enabled_save_area(dcb_addr_parent_handle);
    enabled_area->named.r0 = (uint32_t) argspace_addr_child;

    return err;
    
}

static void spawn_recursive_child_l2_tree_walk(struct spawninfo *si, struct pt_cap_tree_node *node, int is_root) {
    
    static size_t next_slot = 0;
    
    // Reinitialise at root
    if (is_root) {
        next_slot = 0;
    }
    
    // Recurse to the left
    if (node->left != NULL) {
        spawn_recursive_child_l2_tree_walk(si, node->left, 0);
    }
    
    // Build next capref
    struct capref next_cap;
    next_cap.cnode = si->slot_alloc0_ref;
    next_cap.slot = next_slot++;
    
    // Copy the capability
    errval_t err = cap_copy(next_cap, node->cap);
    if (err_is_fail(err)) {
        debug_printf("spawn for %s: %s\n", si->binary_name, err_getstring(err));
    }
    assert(err_is_ok(err));
    
    // Free the slot in the parent cspace
    //  FIXME: Make this work to recuparate slots
    //cap_delete(node->cap);
    //slot_free(node->cap);
    
    // Mutate the capref to reference the child cspace
    next_cap.cnode.croot = CPTR_ROOTCN;
    node->cap = next_cap;
    
    // Recurse to the right
    if (node->right != NULL) {
        spawn_recursive_child_l2_tree_walk(si, node->right, 0);
    }
    
}

static errval_t spawn_invoke_dispatcher(struct spawninfo *si) {
 
    errval_t err = SYS_ERR_OK;

    // Complete process info
    si->pi->name = si->binary_name;
    si->pi->core_id = disp_get_core_id();
    si->pi->dispatcher_cap = &si->child_dispatcher_cap;

    // Register the process
    process_register(si->pi);

    // Invoking the dispatcher
    err = invoke_dispatcher(si->child_dispatcher_cap,
                            cap_dispatcher,
                            si->child_rootcn_cap,
                            si->l1_pt_cap,
                            si->slot_dispframe_cap,
                            true);

    return err;
    
}

static errval_t spawn_cleanup(struct spawninfo *si) {

    errval_t err = SYS_ERR_OK;

    struct parent_mapping *i;
    for (i = si->parent_mappings; i != NULL; i = i->next) {
        err = paging_unmap(get_current_paging_state(), i->addr);
        if (err_is_fail(err)) {
            return err;
        }
        free(i);
    }

    cap_delete(si->child_rootcn_cap);
    slot_free(si->child_rootcn_cap);
    cap_delete(si->child_root_pt_cap);
    slot_free(si->child_root_pt_cap);

    return err;
}

// TODO(M2): Implement this function such that it starts a new process
// TODO(M4): Build and pass a messaging channel to your child process
errval_t spawn_load_by_name(void * cmd, struct spawninfo * si) {

    errval_t err = SYS_ERR_OK;

    // Init spawninfo
    memset(si, 0, sizeof(*si));
    si->pi = (struct process_info *) malloc(sizeof(struct process_info));
    
    // Extract binary name
    si->binary_name = malloc(strlen((char *) cmd));
    if (!si->binary_name) {
        return LIB_ERR_MALLOC_FAIL;
    }
    if (sscanf(cmd, "%s", si->binary_name) != 1) {
        free(si->binary_name);
        return -1;
    }
    si->binary_name = realloc(si->binary_name, strlen(si->binary_name) + 1);
    if (!si->binary_name) {
        return LIB_ERR_MALLOC_FAIL;
    }
    
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
    void *elf_buf = NULL;
    err = paging_map_frame_attr(get_current_paging_state(), &elf_buf, mem->mrmod_size, child_frame, VREGION_FLAGS_READ, NULL, NULL);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed mapping ELF into virtual memory: %s\n", err_getstring(err));
        return err;
    }

    // Add mapping to parent mappings list
    add_parent_mapping(si, elf_buf);

    char *elf = elf_buf;
    //debug_printf("Mapped ELF into memory: 0x%x %c%c%c\n", elf[0], elf[1], elf[2], elf[3]);
    assert(elf[0] == 0x7f && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F');

    // Set up cspace
    err = spawn_setup_cspace(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up cspace: %s\n", err_getstring(err));
        return err;
    }

    // Set up lmp channel
    err = spawn_setup_lmp_channel(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up lmp channel: %s\n", err_getstring(err));
        return err;
    }

    // Set up vspace
    err = spawn_setup_vspace(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up vspace: %s\n", err_getstring(err));
        return err;
    }
    
    // Parse the elf
    err = spawn_parse_elf(si, elf_buf, mem->mrmod_size);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed to parse the ELF: %s\n", err_getstring(err));
        return err;
    }
    
    // Set up the child's dispatcher
    err = spawn_setup_dispatcher(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up the dispatcher: %s\n", err_getstring(err));
        return err;
    }
    
    // Commit fixed vspace allocations
    paging_alloc_fixed_commit(si->child_paging_state);
    
    // Get arguments string from multiboot
    //const char *argstring = multiboot_module_opts(mem);
    
    // Set up the arguments for the child process
    err = spawn_setup_args(si, cmd);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed setting up the arguments: %s\n", err_getstring(err));
        return err;
    }
    
    // Move all L2 cnode capabilities to the cild's cspace
    spawn_recursive_child_l2_tree_walk(si, si->child_paging_state->l2_tree_root, 1);
    
    // Launch dispatcher 🚀
    err = spawn_invoke_dispatcher(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed invoking the dispatcher: %s\n", err_getstring(err));
        return err;
    }

    // Cleanup 
    err = spawn_cleanup(si);
    if (err_is_fail(err)) {
        debug_printf("spawn: Failed cleanup: %s\n", err_getstring(err));
        return err;
    }

    return err;
}
