#ifndef _GRAPHICS_HH_
#define _GRAPHICS_HH_

#include <vector>

#include "Map.hh"
#include "Player.hh"


void init_sdl();
void init_opengl();
void use_map(const TEA::Map &);
void draw_scene(const std::vector<TEA::Player *> &, float, float);

#endif
