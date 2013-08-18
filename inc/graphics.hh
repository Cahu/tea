#ifndef _GRAPHICS_HH_
#define _GRAPHICS_HH_

void init_sdl();
void init_opengl();
void init_world(const char *);
void draw_scene(const std::vector<Player *> &, float, float);

#endif
