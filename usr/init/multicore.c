//
//  multicore.c
//  DoritOS
//

#include "multicore.h"

#include <barrelfish_kpi/types.h>
#include <target/arm/barrelfish_kpi/arm_core_data.h>
#include <aos/kernel_cap_invocations.h>
#include <aos/coreboot.h>
#include <spawn/multiboot.h>
#include <aos/capabilities.h>
#include <elf/elf.h>

#define PRINT_DEBUG 0


extern struct bootinfo *bi;


// Boot the the core with the ID core_id
errval_t boot_core(coreid_t core_id, struct urpc_chan *urpc_chan) {
    
    errval_t err;
    
    struct paging_state *st = get_current_paging_state();
    
    // Allocate frame for relocatable segment of kernel
    struct capref segment_frame_cap;
    size_t segment_size = ARM_CORE_DATA_PAGES * BASE_PAGE_SIZE;
    err = frame_alloc(&segment_frame_cap, segment_size, &segment_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map frame for relocatable segment of kernel
    void *segment_vaddr = NULL;
    err = paging_map_frame(st, &segment_vaddr, segment_size, segment_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Allocate frame for arm_core_data
    struct capref core_data_frame_cap;
    size_t core_data_size = sizeof(struct arm_core_data);
    err = frame_alloc(&core_data_frame_cap, core_data_size, &core_data_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map frame for arm_core_data
    struct arm_core_data *core_data = NULL;
    err = paging_map_frame(st, (void **) (&core_data), core_data_size, core_data_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Allocate frame for init process
    struct capref init_frame_cap;
    size_t init_size = ARM_CORE_DATA_PAGES * BASE_PAGE_SIZE;
    err = frame_alloc(&init_frame_cap, init_size, &init_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map frame for init process
    //    void *init_vaddr = NULL;
    //    err = paging_map_frame(st, &init_vaddr, init_size, init_frame_cap, NULL, NULL);
    //    if (err_is_fail(err)) {
    //        return err;
    //    }
    
    // Allocate frame for URPC
    struct capref urpc_frame_cap;
    size_t urpc_size = URPC_BUF_SIZE;
    err = frame_alloc(&urpc_frame_cap, urpc_size, &urpc_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map frame for URPC
    err = paging_map_frame(st, &urpc_chan->buf, urpc_size, urpc_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Initialize the URPC channel
    urpc_chan_init(urpc_chan, URPC_BSP_BUF_SELECT);
    
#if PRINT_DEBUG
    debug_printf("Set up KCB\n");
#endif
    
    // Allocate RAM for KCB
    struct capref kcb_ram_cap;
    size_t kcb_size = OBJSIZE_KCB;
    err = ram_alloc(&kcb_ram_cap, kcb_size);
    if (err_is_fail(err)) {
        return err;
    }
    
#if PRINT_DEBUG
    debug_printf("Retype KCB capability\n");
#endif
    
    // Retype the RAM cap into a KCB cap
    struct capref kcb_cap;
    err = slot_alloc(&kcb_cap);
    if (err_is_fail(err)) {
        return err;
    }
    err = cap_retype(kcb_cap, kcb_ram_cap, 0, ObjType_KernelControlBlock, 0, 1);
    if (err_is_fail(err)) {
        return err;
    }
    
#if PRINT_DEBUG
    debug_printf("Clone KCB\n");
#endif
    
    // Clone the current KCB and core data
    err = invoke_kcb_clone(kcb_cap, core_data_frame_cap);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Find the cpu_omap44xx module
    struct mem_region *mem = multiboot_find_module(bi, "cpu_omap44xx");
    if (!mem) {
        return SPAWN_ERR_FIND_MODULE;
    }
    
    // Find the init module
    struct mem_region *init_mem = multiboot_find_module(bi, "init");
    if (!init_mem) {
        return SPAWN_ERR_FIND_MODULE;
    }
    
    // Assign init module as monitor module in core data
    core_data->monitor_module.mod_start = init_mem->mr_base;
    core_data->monitor_module.mod_end = init_mem->mr_base + init_mem->mrmod_size;
    core_data->monitor_module.string = init_mem->mrmod_data;
    core_data->monitor_module.reserved = init_mem->mrmod_slot;
    
    // Constructing the capability for the frame containing the ELF image
    struct capref elf_frame = {
        .cnode = cnode_module,
        .slot = mem->mrmod_slot
    };
    
    void *elf_buf = NULL;
    err = paging_map_frame_attr(st, &elf_buf, mem->mrmod_size, elf_frame, VREGION_FLAGS_READ, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Get frame identities
    struct frame_identity core_data_identity;
    err = frame_identify(core_data_frame_cap, &core_data_identity);
    if (err_is_fail(err)) {
        return err;
    }
    
    struct frame_identity segment_frame_identity;
    err = frame_identify(segment_frame_cap, &segment_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }
    
    struct frame_identity init_frame_identity;
    err = frame_identify(init_frame_cap, &init_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }
    
    err = frame_identify(urpc_frame_cap, &urpc_chan->fi);
    if (err_is_fail(err)) {
        return err;
    }
    
    struct frame_identity kcb_frame_identity;
    err = frame_identify(kcb_cap, &kcb_frame_identity);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Set the location of the KCB
    core_data->kcb = kcb_frame_identity.base;
    
    // Set memory region for loading init
    core_data->memory_base_start = init_frame_identity.base;
    core_data->memory_bytes = init_frame_identity.bytes;
    
    // Set the location of the URPC frame
    core_data->urpc_frame_base = (uint32_t) urpc_chan->fi.base;
    core_data->urpc_frame_size = (uint32_t) urpc_chan->fi.bytes;
    
    // Set the name of the init process
    strcpy(core_data->init_name, "init");
    
    // Set the commandline for the new kernel
    core_data->cmdline = init_mem->mr_base + init_mem->mrmod_data;
    
    err = load_cpu_relocatable_segment(elf_buf, segment_vaddr, segment_frame_identity.base, core_data->kernel_load_base, &core_data->got_base);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Clean and invalidate cache
    sys_armv7_cache_clean_poc((void *) ((uint32_t) segment_frame_identity.base), (void *) ((uint32_t) segment_frame_identity.base + (uint32_t) segment_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) segment_frame_identity.base), (void *) ((uint32_t) segment_frame_identity.base + (uint32_t) segment_frame_identity.bytes));
    sys_armv7_cache_clean_poc((void *) ((uint32_t) init_frame_identity.base), (void *) ((uint32_t) init_frame_identity.base + (uint32_t) init_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) init_frame_identity.base), (void *) ((uint32_t) init_frame_identity.base + (uint32_t) init_frame_identity.bytes));
    sys_armv7_cache_invalidate((void *) ((uint32_t) urpc_chan->fi.base), (void *) ((uint32_t) urpc_chan->fi.base + (uint32_t) urpc_chan->fi.bytes));
    
    // Structured buffer for message to send
    struct urpc_bi_caps msg;
    
    // Get bootinfo frame capability
    struct capref bi_cap = {
        .cnode = {
            .croot = CPTR_ROOTCN,
            .cnode = CPTR_TASKCN_BASE,
            .level = CNODE_TYPE_OTHER
        },
        .slot = TASKCN_SLOT_BOOTINFO
    };
    
    // Write the bootinfo frame identity to the sending buffer
    err = frame_identify(bi_cap, &msg.bootinfo);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Get mmstrings frame capability
    struct capref mmstrings_cap = {
        .cnode = cnode_module,
        .slot = 0
    };
    
    // Write the mmstrings frame identity to the sending buffer
    err = frame_identify(mmstrings_cap, &msg.mmstrings_cap);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Write the module frame identites to the sending buffer
    size_t index = 0;
    for (int i = 0; i < bi->regions_length; i++) {
        
        if (bi->regions[i].mr_type == RegionType_Module) {
            
            msg.modules[index].slot = bi->regions[i].mrmod_slot;
            
            // Constructing the capability reference
            struct capref frame_cap = {
                .cnode = cnode_module,
                .slot = bi->regions[i].mrmod_slot
            };
            
            // Write the frame identitiy
            err = frame_identify(frame_cap, &msg.modules[index].fi);
            if (err_is_fail(err)) {
                return err;
            }
            
            index++;
            
        }
        
    }
    msg.num_modules = index;
    
    // Send the message to the app cpu
    urpc_send(urpc_chan,
              &msg,
              sizeof(struct urpc_bi_caps)+ msg.num_modules * sizeof(struct module_frame_identity),
              URPC_MessageType_Bootinfo);
    
    
#if PRINT_DEBUG
    debug_printf("Booting core\n");
#endif
    
    // Boot the code
    invoke_monitor_spawn_core(1, CPU_ARM7, core_data_identity.base);
    
#if PRINT_DEBUG
    debug_printf("Booted core\n");
#endif
    
    return err;
}

// Forge all the needed module and RAM capabilites based on bootinfo
errval_t forge_module_caps(struct urpc_bi_caps *bi_frame_identities, coreid_t my_core_id) {
    
    assert(bi_frame_identities != NULL);
    
    errval_t err = SYS_ERR_OK;
    
    // Create modules cnode
    struct capref cnode_module_ref = {
        .cnode = {
            .croot = CPTR_ROOTCN,
            .cnode = 0,
            .level = CNODE_TYPE_ROOT
        },
        .slot = ROOTCN_SLOT_MODULECN
    };
    cslot_t retslots;
    err = cnode_create_raw(cnode_module_ref,
                           &cnode_module,
                           ObjType_L2CNode,
                           L2_CNODE_SLOTS,
                           &retslots);
    assert(retslots == L2_CNODE_SLOTS);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Forge the modules string area capability
    struct capref mmstrings_cap = {
        .cnode = cnode_module,
        .slot = 0
    };
    err = frame_forge(mmstrings_cap,
                      bi_frame_identities->mmstrings_cap.base,
                      bi_frame_identities->mmstrings_cap.bytes,
                      my_core_id);
    if (err_is_fail(err)) {
        return err;
    }

    // Iterate the received frame identities
    for (int i = 0; i < bi_frame_identities->num_modules; i++) {

        // Construct the capability reference
        struct capref frame_cap = {
            .cnode = cnode_module,
            .slot = bi_frame_identities->modules[i].slot
        };
        
        // Forge a frame for the received frame identity
        err = frame_forge(frame_cap,
                          bi_frame_identities->modules[i].fi.base,
                          bi_frame_identities->modules[i].fi.bytes,
                          my_core_id);
        if (err_is_fail(err)) {
            return err;
        }

    }
    
    return err;

}
