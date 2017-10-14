#include <aos/aos.h>
#include <spawn/spawn.h>

#include <elf/elf.h>
#include <aos/dispatcher_arch.h>
#include <barrelfish_kpi/paging_arm_v7.h>
#include <barrelfish_kpi/domain_params.h>
#include <spawn/multiboot.h>

extern struct bootinfo *bi;

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

    return err;
}
