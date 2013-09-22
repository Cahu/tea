#ifndef _PLAYERUTILS_HH_
#define _PLAYERUTILS_HH_

#include "Map.hh"
#include "Player.hh"

#include <vector>


void tick_players(
	const std::vector<TEA::Player *> &,
	const TEA::Map &,
	unsigned int
);

#endif
