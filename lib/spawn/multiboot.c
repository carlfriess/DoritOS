
#include <aos/aos.h>
#include <assert.h>
#include <string.h>

#include <spawn/multiboot.h>

static char * multiboot_strings;

const char *multiboot_module_opts(struct mem_region *module)
{
    assert(module != NULL);
    assert(module->mr_type == RegionType_Module);

    const char *optstring = multiboot_module_rawstring(module);

    // find the first space (or end of string if there is none)
    const char *args = strchr(optstring, ' ');
    if (args == NULL) {
        args = optstring + strlen(optstring);
    }

    // search backward for last '/' before the first ' '
    for (const char *c = args; c > optstring; c--) {
        if (*c == '/') {
            return c + 1;
        }
    }

    return optstring;
}
/// Returns a raw pointer to the modules string area string
const char *multiboot_module_rawstring(struct mem_region *region)
{
    if (multiboot_strings == NULL) {
        errval_t err;
        /* Map in multiboot module strings area */
        struct capref mmstrings_cap = {
            .cnode = cnode_module,
            .slot = 0
        };

        err = paging_map_frame_attr(get_current_paging_state(),
            (void **)&multiboot_strings, BASE_PAGE_SIZE, mmstrings_cap,
            VREGION_FLAGS_READ, NULL, NULL);

        if (err_is_fail(err)) {
            DEBUG_ERR(err, "vspace_map failed");
	        return NULL;
        }
#if 0
        printf("Mapped multiboot_strings at %p\n", multiboot_strings);
        for (int i = 0; i < 256; i++) {
            if ((i & 15) == 0) printf("%04x  ", i);
            printf ("%02x ", multiboot_strings[i]& 0xff);
            if ((i & 15) == 15) printf("\n");
        }
#endif
    }

    if (region == NULL || region->mr_type != RegionType_Module) {
        return NULL;
    }
    return multiboot_strings + region->mrmod_data;
}

/// returns the basename without arguments of a multiboot module
// XXX: returns pointer to static buffer. NOT THREAD SAFE
const char *multiboot_module_name(struct mem_region *region)
{
    const char *str = multiboot_module_rawstring(region);
    if (str == NULL) {
	return NULL;
    }

    // copy module data to local buffer so we can mess with it
    static char buf[128];
    strncpy(buf, str, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0'; // ensure termination

    // ignore arguments for name comparison
    char *args = strchr(buf, ' ');
    if (args != NULL) {
        *args = '\0';
    }

    return buf;
}

struct mem_region *multiboot_find_module(struct bootinfo *bi, const char *name)
{

    for(size_t i = 0; i < bi->regions_length; i++) {
        struct mem_region *region = &bi->regions[i];
        const char *modname = multiboot_module_name(region);
        if (modname != NULL &&
            strncmp(modname + strlen(modname) - strlen(name),
                    name, strlen(name)) == 0) {
            return region;
        }
    }

    return NULL;
}
