#ifndef _GRAPHICS_HH_
#define _GRAPHICS_HH_

#include <vector>

#include "Player.hh"


void init_sdl();
void init_opengl();
void init_world(const char *);
void draw_scene(const std::vector<TEA::Player *> &, float, float);

#endif
