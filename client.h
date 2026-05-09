struct state_t* init(int width, int height);
void install_frame_drawer(struct state_t* state, void (*frame_drawer)(void* pixel_buffer));
int dispatch_events(struct state_t* state);
void disconnect(struct state_t* state);