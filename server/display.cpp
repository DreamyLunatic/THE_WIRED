#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

/* ---- globals we bind from the registry ---- */
static struct wl_compositor *compositor = NULL;
static struct wl_shm        *shm        = NULL;
static struct xdg_wm_base   *wm_base    = NULL;

static int   width = 640, height = 480;
static int   running = 1;

/* ---- create an anonymous shared-memory file ---- */
static int create_shm_file(size_t size) {
    char name[] = "/wl_shm-XXXXXX";
    /* randomize the name a bit */
    for (char *p = name + 8; *p; p++) *p = 'A' + (rand() % 26);

    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) return -1;
    shm_unlink(name);
    if (ftruncate(fd, size) < 0) { close(fd); return -1; }
    return fd;
}

/* ---- draw a buffer and return it ---- */
static struct wl_buffer *draw_frame(void) {
    int stride = width * 4;
    int size   = stride * height;

    int fd = create_shm_file(size);
    if (fd < 0) return NULL;

    uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { close(fd); return NULL; }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(
        pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    /* fill: simple checkerboard so you can see it's real pixels */
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            data[y * width + x] =
                ((x / 32 + y / 32) % 2) ? 0xFF1E1E2E : 0xFF89B4FA;

    munmap(data, size);
    return buffer;
}

/* ================= xdg_wm_base: must answer pings ================= */
static void wm_base_ping(void *d, struct xdg_wm_base *b, uint32_t serial) {
    (void)d;
    xdg_wm_base_pong(b, serial);
}
static const struct xdg_wm_base_listener wm_base_listener = { .ping = wm_base_ping };

/* ================= xdg_surface: configure handshake ================= */
static void xdg_surface_configure(void *d, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
    struct wl_surface *surface = d;
    xdg_surface_ack_configure(xdg_surface, serial);

    struct wl_buffer *buffer = draw_frame();
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);
}
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure
};

/* ================= xdg_toplevel: close + resize ================= */
static void toplevel_configure(void *d, struct xdg_toplevel *t,
                               int32_t w, int32_t h, struct wl_array *states) {
    (void)d; (void)t; (void)states;
    if (w > 0 && h > 0) { width = w; height = h; }
}
static void toplevel_close(void *d, struct xdg_toplevel *t) {
    (void)d; (void)t;
    running = 0;
}
static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = toplevel_configure,
    .close     = toplevel_close,
};

/* ================= registry: bind the globals we need ================= */
static void registry_global(void *d, struct wl_registry *reg, uint32_t name,
                            const char *iface, uint32_t version) {
    (void)d; (void)version;
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
    } else if (strcmp(iface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
    } else if (strcmp(iface, xdg_wm_base_interface.name) == 0) {
        wm_base = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wm_base, &wm_base_listener, NULL);
    }
}
static void registry_global_remove(void *d, struct wl_registry *r, uint32_t n) {
    (void)d; (void)r; (void)n;
}
static const struct wl_registry_listener registry_listener = {
    .global        = registry_global,
    .global_remove = registry_global_remove,
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) { fprintf(stderr, "cannot connect to Wayland display\n"); return 1; }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);   /* receive the globals */

    if (!compositor || !shm || !wm_base) {
        fprintf(stderr, "missing required Wayland globals\n");
        return 1;
    }

    /* surface -> xdg_surface -> xdg_toplevel */
    struct wl_surface  *surface  = wl_compositor_create_surface(compositor);
    struct xdg_surface *xsurface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xsurface, &xdg_surface_listener, surface);

    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xsurface);
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, NULL);
    xdg_toplevel_set_title(toplevel, "Raw Wayland Window");
    xdg_toplevel_set_app_id(toplevel, "raw-wayland");

    wl_surface_commit(surface);      /* triggers first configure */

    while (running && wl_display_dispatch(display) != -1)
        ;                            /* event loop */

    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(xsurface);
    wl_surface_destroy(surface);
    wl_display_disconnect(display);
    return 0;
}

