#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <wayland-server.h>

struct wl_event_loop* get_event_loop();
int event_loop_init(struct wl_display* d);

#endif
