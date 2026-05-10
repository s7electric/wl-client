#include "client.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define WIDTH 400
#define HEIGHT 400
#define FPS 20

#define log(message) fprintf(stderr, message)

static float dtheta = 2*M_PI/3;
static float r = WIDTH/4;
float angle = 0;

void put_pixels(void* pixel_buffer) {
    Olivec_Canvas oc = olivec_canvas(
        (uint32_t*) pixel_buffer,
        WIDTH,
        HEIGHT,
        WIDTH
    );
    float cx = WIDTH/2;
    float cy = HEIGHT/2;
    log("Created oc canvas\n");
    olivec_fill(oc, 0xFFFFFFFF);
    log("Filled oc canvas\n");

    olivec_triangle3c(
        oc,
        cx + r*cosf(angle), cy - r*sinf(angle),
        cx + r*cosf(angle + dtheta), cy - r*sinf(angle + dtheta),
        cx + r*cosf(angle + 2*dtheta), cy - r*sinf(angle + 2*dtheta),
        0xFFFF0000,
        0xFF00FF00,
        0xFF0000FF
    );
    // olivec_circle(oc, cx, cy, 100, 0xFFFF0000);
    log("Drew object on oc canvas\n");
}

int main() {
    struct state_t* state = init(WIDTH, HEIGHT);
    install_frame_drawer(state, put_pixels);

    while(dispatch_events(state)) {
        angle += 0.05;
        request_new_frame(state);
        log("Incremented angle\n");
    }

    disconnect(state);
    return 0;
}
