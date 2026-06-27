#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <vector>
#include <cstdio>
#include "event-handler.h"

// TODO: Figure out what the fuck is that
struct Surface {
    wl_surface *surface;
};

// TODO: Figure out what the fuck is that
struct Output {
    wl_output *output;
};

// TODO: Figure out what the fuck is that
struct Seat {
    wl_seat *seat;
};

struct Compositor {
    // --- libwayland core ---
    wl_display    *display;
    wl_event_loop *event_loop;          // = wl_display_get_event_loop(display)

    // --- globals you advertise to clients ---
    wl_global *compositor;              // wl_compositor: clients create surfaces & regions
    wl_global *subcompositor;           // wl_subcompositor: subsurfaces
    wl_global *xdg_wm_base;             // xdg-shell: toplevels & popups
    wl_global *seat;                    // wl_seat: keyboard/pointer/touch
    wl_global *data_device_manager;     // clipboard + drag-and-drop
    // wl_shm: don't make this yourself — call wl_display_init_shm(display)
    // wl_output: one global per monitor, owned by each Output below

    // --- your bookkeeping ---
    std::vector<Surface*> surfaces;     // every wl_surface a client created
    std::vector<Output*>  outputs;      // one per monitor (each owns a wl_global)
    std::vector<Seat*>    seats;        // usually just one

    // --- input routing state ---
    Surface *keyboard_focus;
    Surface *pointer_focus;
    double   pointer_x, pointer_y;

    // --- your output/render backend (YOU pick and build this) ---
    // FIX: Backend *backend;   // DRM+GBM+EGL on bare metal, or nested in X11/Wayland, or SHM
};

struct my_state {
    struct wl_display *display;

    // the single output, inline for now
    int32_t width, height;            // current mode, px
    int32_t refresh;                  // mHz
    int32_t phys_width, phys_height;  // mm
    enum wl_output_transform transform;
};

static void
wl_output_handle_bind(struct wl_client *client, void *data,
    uint32_t version, uint32_t id)
{
    struct my_state *state = static_cast<struct my_state *>(data); 
    // TODO
}


int
main(int argc, char *argv[])
{
    struct wl_display *display = wl_display_create();
    if (!display) {
        fprintf(stderr, "Unable to create Wayland display.\n");
        return 1;
    }

    const char *socket = wl_display_add_socket_auto(display);
    if (!socket) {
        fprintf(stderr, "Unable to add socket to Wayland display.\n");
        return 1;
    }

    struct my_state state = {};
    state.display = display;
    state.width = 1920;
    state.height = 1080;

    wl_global_create(display, &wl_output_interface,
        1, &state, wl_output_handle_bind);

    // Advertise wl_shm so clients see at least one global in the registry.
    if (wl_display_init_shm(display) != 0) {
        fprintf(stderr, "Unable to initialize wl_shm.\n");
        return 1;
    }

    if (event_loop_init(display) == 1) {
        fprintf(stderr, "Unable to initialize event loop.\n");
        return 1;
    }

    fprintf(stderr, "Running Wayland display on socket %s\n", socket);
    wl_display_run(display);

    wl_display_destroy(display);
    return 0;
}


