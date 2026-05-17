#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "xdg.h"
#include "shm.h"
#include "client.h"

#define log(message) fprintf(stderr, message)
#define VERSION_MIN (version > 4 ? version : 4)
#define PIXEL_SIZE 4

struct buffer_t {
    struct wl_buffer* wl_buffer;
    uint32_t* data;
    bool busy;
};

struct state_t {
    uint32_t width;
    uint32_t height;
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct wl_shm* shm;
    uint32_t format;
    struct wl_shm_pool* shm_pool;
    struct buffer_t buffer1;
    struct buffer_t buffer2;
    struct buffer_t pending_buffer;
    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface* xdg_surface;
    struct xdg_toplevel* xdg_toplevel;
    uint32_t last_frame_ms;
    bool can_request_frame;
    void (*draw_frame)(void* pixel_buffer);
};


void choose_buffer_and_commit(struct state_t* state) {
    struct buffer_t* working_buffer;
    if (!state->buffer1.busy) {
        working_buffer = &state->buffer1;
        log("Writing to buffer 1\n");
    }
    else {
        working_buffer = &state->buffer2;
        log("Writing to buffer 2\n");
    }
    state->draw_frame(working_buffer->data);
    log("Called draw_frame function\n");
    wl_surface_attach(state->surface, working_buffer->wl_buffer, 0, 0);
    wl_surface_damage_buffer(state->surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(state->surface);
    working_buffer->busy = true;
}


//####################
// wl_display_handlers
//####################
static void wl_display_handle_error(void* data, struct wl_display* display, void* object, uint32_t code, const char* errormsg) {
    fprintf(stderr, "ERROR RECEIVED: code %d: %s", code, errormsg);
}
static const struct wl_display_listener display_listener = {
    .error = wl_display_handle_error
};


//################
// wl_shm handlers
//################
static void shm_handle_format(void* data, struct wl_shm* shm, uint32_t format) {
    struct state_t* state = data;
    printf("Got shm format %d\n", format);
    state->format = 0; // don't care - use 32-bit ARGB
}
static const struct wl_shm_listener shm_listener = {
    .format = shm_handle_format
};


//###################
// wl_buffer handlers
//###################
static void buffer_handle_release(void* data, struct wl_buffer* buffer) {
    struct state_t* state = data;
    if (buffer == state->buffer1.wl_buffer) {
        state->buffer1.busy = false;
        log("Releasing buffer 1\n");
    }
    else {
        state->buffer2.busy = false;
        log("Releasing buffer 2\n");
    }
}
static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_handle_release
};


//#####################
// xdg_wm_base handlers
//#####################
static void xdg_wm_base_handle_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    struct state_t* state = data;
    log("Ping\n");
    xdg_wm_base_pong(state->xdg_wm_base, serial);
    log("Pong\n");
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_handle_ping
};


//#####################
// wl_callback handlers
//#####################
static void wl_callback_handle_done(void* data, struct wl_callback* callback, uint32_t callback_data) {
    log("Received 'done' callback\n");
    wl_callback_destroy(callback);
    struct state_t* state = data;
    choose_buffer_and_commit(state);
    state->can_request_frame = true;
    state->last_frame_ms = callback_data;
}
static struct wl_callback_listener callback_listener = {
    .done = wl_callback_handle_done
};


//#####################
// xdg_surface_handlers
//#####################
static void xdg_surface_handle_configure(void* data, struct xdg_surface *xdg_surface, uint32_t serial) {
    log("Received configure event, configuring\n");
    struct state_t* state = data;
    xdg_surface_ack_configure(state->xdg_surface, serial);    
    choose_buffer_and_commit(state);
    state->can_request_frame = true;
}
static struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure
};


//#####################
// wl_registry handlers
//#####################
static void registry_handle_globals(void* data, struct wl_registry* registry, 
                                uint32_t name, const char* interface, uint32_t version) {
    printf("interface: %s, version: %d, name: %d\n", interface, version, name);
    struct state_t* state = data;
    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(
            state->registry, 
            name, 
            &wl_compositor_interface, 
            version
        );
    }
    else if (!strcmp(interface, wl_shm_interface.name)) {
        state->shm = wl_registry_bind(
            state->registry, 
            name, 
            &wl_shm_interface, 
            version
        );
        wl_shm_add_listener(
            state->shm, 
            &shm_listener, 
            data
        );
    }
    else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        state->xdg_wm_base = wl_registry_bind(
            state->registry, 
            name, 
            &xdg_wm_base_interface, 
            1
        );
        xdg_wm_base_add_listener(
            state->xdg_wm_base,
            &xdg_wm_base_listener, 
            data
        );
    }
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_globals
};


struct state_t* init(int width, int height) {
    struct state_t* state = malloc(sizeof(struct state_t));
    state->can_request_frame = false;
    state->display = wl_display_connect(NULL);
    if (state->display == NULL) {
        log("Failed to connect to display\n");
        return NULL;
    }
    wl_display_add_listener(state->display, &display_listener, state);
    log("Connected to display\n");

    state->registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(
        state->registry, 
        &registry_listener, 
        state
    );
    wl_display_roundtrip(state->display);
    log("Got handle to registry\n");
    log("Got handle to compositor & shm & xdg_wm_base\n");

    state->surface = wl_compositor_create_surface(state->compositor);
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->surface);
    xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    xdg_toplevel_set_title(state->xdg_toplevel, "Hello World");
    xdg_toplevel_set_app_id(state->xdg_toplevel, "test-window");
    log("Created xdg_surface and xdg_toplevel\n");
    wl_surface_commit(state->surface);
    
    /* Create and map shared memory space */
    state->width = width;
    state->height = height;
    const int pixel_size = PIXEL_SIZE; // ARGB 32 bit hardcoded
    const int pool_size = 2 * state->height * state->width * pixel_size;
    int shm_fd = create_shm_file(pool_size);
    if (shm_fd == -1) {
        log("Failed to create shm file, terminating client\n");
        return NULL;
    }
    else {
        log("Created shm file\n");
    }
    uint8_t* pixel_data = mmap(
        NULL, 
        pool_size, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        shm_fd, 
        0
    );
    if (pixel_data == MAP_FAILED) {
        log("Failed to map shm file, bailing out\n");
        return NULL;
    }
    else {
        log("Mapped shm file\n");
    }
    state->shm_pool = wl_shm_create_pool(state->shm, shm_fd, pool_size);
    log("Created shm pool\n");

    /* Initialize two buffers*/
    struct buffer_t buffer1 = {0}, buffer2 = {0};
    state->buffer1 = buffer1;
    state->buffer2 = buffer2;
    int offset = pool_size / 2;
    // Buffer 1
    state->buffer1.wl_buffer = wl_shm_pool_create_buffer(
        state->shm_pool, 
        0, 
        state->width, 
        state->height, 
        state->width * pixel_size, 
        WL_SHM_FORMAT_ARGB8888
    );
    state->buffer1.data = (uint32_t*) pixel_data;
    state->buffer1.busy = false;
    wl_buffer_add_listener(state->buffer1.wl_buffer, &buffer_listener, state);
    log("Created buffer 1\n");
    // Buffer 2 @ offset
    state->buffer2.wl_buffer = wl_shm_pool_create_buffer(
        state->shm_pool, 
        offset, 
        state->width, 
        state->height, 
        state->width * pixel_size, 
        WL_SHM_FORMAT_ARGB8888
    );
    state->buffer2.data = (uint32_t*) &pixel_data[offset];
    state->buffer2.busy = false;
    wl_buffer_add_listener(state->buffer2.wl_buffer, &buffer_listener, state);
    log("Created buffer 2\n");
    return state;
}

void install_frame_drawer(struct state_t* state, void (*frame_drawer)(void* pixel_buffer)) {
    state->draw_frame = frame_drawer;
}

void request_new_frame(struct state_t* state) {
    if (state->can_request_frame) {
        state->can_request_frame = false;
        struct wl_callback* cb = wl_surface_frame(state->surface);
        log("Sent frame request\n");
        wl_callback_add_listener(cb, &callback_listener, state);
    }
    else {
        log("Failed to request a frame\n");
    }
}

int get_last_frame_time_ms(struct state_t* state) {
    return state->last_frame_ms;
}

int dispatch_events(struct state_t* state) {
    return wl_display_dispatch(state->display);
}

void disconnect(struct state_t* state) {
    log("Disconnected from display\n");
    wl_display_disconnect(state->display);
}
