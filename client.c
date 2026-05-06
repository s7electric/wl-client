#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "shm.h"

#define log(message) fprintf(stderr, message)
#define VERSION_MIN (version > 4 ? version : 4)

struct buffer_t {
    struct wl_buffer* buffer;
    uint32_t* data;
    bool busy;
};

struct state_t {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct wl_shm* shm;
    uint32_t format;
    struct wl_shm_pool* shm_pool;
    struct buffer_t buffer1;
    struct buffer_t buffer2;
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


//#####################
// wl_registry handlers
//#####################
static void registry_handle_global(void* data, struct wl_registry* registry, 
                                uint32_t name, const char* interface, uint32_t version) {
    struct state_t* state = data;
    printf("interface: %s, version: %d, name: %d\n", interface, version, name);
    
    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(state->registry, name, &wl_compositor_interface, version);
    
    }
    else if (!strcmp(interface, wl_shm_interface.name)) {
        state->shm = wl_registry_bind(
            state->registry, 
            name, 
            &wl_shm_interface, 
            version);
        wl_shm_add_listener(state->shm, &shm_listener, data);
    }
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global
};

int main(int argc, char** argv) {
    struct state_t state = {0};
    state.display = wl_display_connect(NULL);
    if (state.display == NULL) {
        log("Failed to connect to display\n");
        return 1;
    }
    log("Connected to display\n");

    state.registry = wl_display_get_registry(state.display);
    log("Got handle to registry\n");

    wl_registry_add_listener(
        state.registry, 
        &registry_listener, 
        &state
    );
    wl_display_roundtrip(state.display);
    log("Got handle to compositor, shm\n");
    wl_display_roundtrip(state.display);
    log("Read supported colour formats\n");
    
    const int width = 1280;
    const int height = 720;
    const int pixel_size = 4; // ARGB 32 bit
    const int pool_size = 2 * height * width * pixel_size;

    int shm_fd = create_shm_file(pool_size);
    uint8_t* data = mmap(
        NULL, 
        pool_size, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        shm_fd, 
        0
    );
    state.shm_pool = wl_shm_create_pool(state.shm, shm_fd, pool_size);

    struct buffer_t buffer1, buffer2;
    state.buffer1 = buffer1;
    state.buffer2 = buffer2;
    int offset = pool_size / 2;
    state.buffer1.buffer = wl_shm_pool_create_buffer(
        state.shm_pool, 
        offset, 
        width, 
        height, 
        width * pixel_size, 
        WL_SHM_FORMAT_ABGR8888
    );
    state.buffer1.data = (uint32_t*)&data[offset];

    
    state.surface = wl_compositor_create_surface(state.compositor);


    wl_display_disconnect(state.display);
    log("Disconnected from display\n");
}