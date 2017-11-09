//
//  multicore.c
//  DoritOS
//

#include "multicore.h"

#include <stdint.h>

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
errval_t boot_core(coreid_t core_id, void **urpc_frame) {
    
    errval_t err;
    
    struct paging_state *st = get_current_paging_state();
    
    // Allocate frame for relocatable segment of kernel
    struct capref segment_frame_cap;
    size_t segment_size = 1100 * BASE_PAGE_SIZE;
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
    size_t urpc_size = MON_URPC_SIZE;
    err = frame_alloc(&urpc_frame_cap, urpc_size, &urpc_size);
    if (err_is_fail(err)) {
        return err;
    }
    
    // Map frame for URPC
    err = paging_map_frame(st, urpc_frame, urpc_size, urpc_frame_cap, NULL, NULL);
    if (err_is_fail(err)) {
        return err;
    }
    
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
    
    struct frame_identity urpc_frame_identity;
    err = frame_identify(urpc_frame_cap, &urpc_frame_identity);
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
    core_data->urpc_frame_base = (uint32_t) urpc_frame_identity.base;
    core_data->urpc_frame_size = (uint32_t) urpc_frame_identity.bytes;
    
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
    sys_armv7_cache_invalidate((void *) ((uint32_t) urpc_frame_identity.base), (void *) ((uint32_t) urpc_frame_identity.base + (uint32_t) urpc_frame_identity.bytes));
    
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


/* MARK: - ========== URPC ========== */

// Initialize a URPC frame
void urpc_frame_init(void *urpc_frame) {
    
    // Set the counters to zero
    (*(uint32_t *)(urpc_frame + URPC_APP_RX_OFFSET)) = 0;
    (*(uint32_t *)(urpc_frame + URPC_APP_TX_OFFSET)) = 0;
    
}

// On BSP: Get pointer to the memory region, where to write the message to be sent
void *get_urpc_send_to_app_buf(void *urpc_frame) {
    return urpc_frame + URPC_BSP_TX_OFFSET + sizeof(uint32_t);
}

// On APP: Get pointer to the memory region, where to write the message to be sent
void *get_urpc_send_to_bsp_buf(void *urpc_frame) {
    return urpc_frame + URPC_APP_TX_OFFSET + sizeof(uint32_t);
}

// Send a message to the APP cpu
void send_to_app(void *urpc_frame) {
    
    (*(uint32_t *)(urpc_frame + URPC_BSP_TX_OFFSET))++;
    
}

// Send a message to the BSP cpu
void send_to_bsp(void *urpc_frame) {
    
    (*(uint32_t *)(urpc_frame + URPC_APP_TX_OFFSET))++;
    
}

