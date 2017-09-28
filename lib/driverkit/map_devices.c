/**
 * \brief Memory management helper functions for device drivers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <aos/aos.h>
#include <aos/capabilities.h>
#include <aos/aos_rpc.h>

#include <driverkit/driverkit.h>

#define UNBITS_GENPA(bits) (((genpaddr_t)1) << (bits))

/**
 * \brief Maps device register. Requests needed capability via RPC 
 * using aos_rpc_get_device_cap.
 * 
 * The function is used mostly as a helper to map registers by drivers
 * 
 * \param[in] address The address of the device region you want to map.
 * \param[in] size The size of the region.
 * \param[out] return_address The virtual memory address where the region
 * was mapped at.
 *
 * \retval SYS_ERR_OK Mapping was succesful.
 */
errval_t map_device_register(lpaddr_t address, size_t size, lvaddr_t *return_address)
{
    errval_t err;

    struct capref device_cap;

    size = (size + BASE_PAGE_SIZE - 1) & ~(BASE_PAGE_SIZE-1);
    lpaddr_t address_base = address & ~(BASE_PAGE_SIZE-1);
    lpaddr_t offset = address & (BASE_PAGE_SIZE-1);

    err = aos_rpc_get_device_cap(get_init_rpc(), address_base, size, &device_cap);
    if (err_is_fail(err)) {
        return err;
    }

    struct frame_identity id;
    invoke_frame_identify(device_cap, &id);
    debug_printf("got cap: %x %x\n", id.base, id.bytes);

    void* frame_base;
    err = paging_map_frame_attr(get_current_paging_state(), &frame_base,
                                size, device_cap, VREGION_FLAGS_READ_WRITE_NOCACHE,
                                NULL, NULL);
    *return_address = (lvaddr_t)frame_base + offset;

    return err;
}
