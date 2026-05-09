#include "client.h"
#include <stdio.h>
#include <unistd.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define WIDTH 400
#define HEIGHT 400

#define log(message) fprintf(stderr, message)

static Olivec_Canvas oc;

void put_pixels(void* pixel_buffer) {
    Olivec_Canvas oc = olivec_canvas(
        (uint32_t*) pixel_buffer,
        WIDTH,
        HEIGHT,
        WIDTH
    );
    int cx = WIDTH/2;
    int cy = HEIGHT/2;
    log("Created oc canvas\n");
    olivec_fill(oc, 0xFFFFFFFF);
    log("Filled oc canvas\n");

    olivec_triangle3c(
        oc,
        cx - WIDTH/4, cy + HEIGHT/4,
        cx + WIDTH/4, cy + HEIGHT,
        cx, cy - HEIGHT/4,
        0xFFFF0000,
        0xFF00FF00,
        0xFF0000FF
    );
    log("Drew triangle on oc canvas\n");
}

int main() {
    struct state_t* state = init(WIDTH, HEIGHT);
    install_frame_drawer(state, put_pixels);

    while(dispatch_events(state));
    
    disconnect(state);
    return 0;
}
