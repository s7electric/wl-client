#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <bits/types/struct_timeval.h>

#include "wlclient.h"

#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define WIDTH 400
#define HEIGHT 400
#define FPS 144
#define RPS 1.5
#define delta_time_us (suseconds_t)(1000 * 1000 / FPS)

#define log(message) fprintf(stderr, message)

static float dtheta = 2*M_PI/3;
static float r = (float)WIDTH/4;
float angle = 0;

void put_pixels(void* pixel_buffer) {
    Olivec_Canvas oc = olivec_canvas(
        (uint32_t*) pixel_buffer,
        WIDTH,
        HEIGHT,
        WIDTH
    );
    float cx = (float)WIDTH/2;
    float cy = (float)HEIGHT/2;
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

    struct timeval tv;
    suseconds_t prev_frame_time, curr_time;
    gettimeofday(&tv, NULL);
    prev_frame_time = tv.tv_usec;
    request_new_frame(state);

    while(dispatch_events(state)) {
        angle += (2*M_PI/FPS) * RPS;
        log("Incremented angle\n");

        gettimeofday(&tv, NULL);
        curr_time = tv.tv_usec;
        if ((curr_time - prev_frame_time) < delta_time_us) {
            usleep(delta_time_us - (curr_time - prev_frame_time));
        }
        gettimeofday(&tv, NULL);
        prev_frame_time = tv.tv_usec;
        request_new_frame(state);
    }

    disconnect(state);
    return 0;
}
