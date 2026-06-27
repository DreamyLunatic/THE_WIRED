#include <cstdio>
#include <wayland-server.h>
#include "event-handler.h"

static struct wl_event_loop* event_loop;
static struct wl_event_source* heartbeat_source;
static struct wl_listener client_created_listener;

struct wl_event_loop* get_event_loop() { return event_loop; }

static int
heartbeat(void* data)
{
    fprintf(stderr, "Server heartbeat: event loop is alive.\n");
    wl_event_source_timer_update(heartbeat_source, 5000);
    return 0;
}

static void
on_client_created(struct wl_listener* listener, void* data)
{
    fprintf(stderr, "Client connected.\n");
}

int
event_loop_init(struct wl_display* d)
{
    event_loop = wl_display_get_event_loop(d);
    if (!event_loop)
        return 1;

    heartbeat_source = wl_event_loop_add_timer(event_loop, heartbeat, nullptr);
    wl_event_source_timer_update(heartbeat_source, 5000);

    client_created_listener.notify = on_client_created;
    wl_display_add_client_created_listener(d, &client_created_listener);
    
    return 0;
}
