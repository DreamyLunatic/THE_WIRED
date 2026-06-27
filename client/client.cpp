#include <cstdio>
#include <wayland-client.h>

static void
registry_global(void *data, struct wl_registry *registry, uint32_t name,
                const char *interface, uint32_t version)
{
    printf("Global %u: %s (version %u)\n", name, interface, version);
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove,
};

int
main(int argc, char *argv[])
{
    // Connects to the socket named by WAYLAND_DISPLAY (e.g. wayland-0).
    struct wl_display *display = wl_display_connect("wayland-0");
    if (!display) {
        fprintf(stderr, "Unable to connect to Wayland display.\n");
        return 1;
    }
    printf("Connected to Wayland display.\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, nullptr);

    // Block until the server has answered, so all globals get printed.
    wl_display_roundtrip(display);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    printf("Disconnected.\n");
    return 0;
}
