#include <cstdio>
#include "playerutils.hh"

using TEA::Map;
using TEA::Player;

#define EPSILON 0.01


void tick_players(
	const std::vector<TEA::Player *> &players,
	const TEA::Map &map,
	unsigned int delay
) {
	for (unsigned int i = 0; i < players.size(); i++) {
		Player *p = players[i];

		if (p == NULL) {
			continue;
		}

		// First simulate a move without taking potential obstacles into
		// consideration. We will check obstacles on the path afterwards and
		// correct the new position.
		Coor c1(p->get_xpos(), p->get_ypos());
		p->tick(delay);
		Coor c2(p->get_xpos(), p->get_ypos());

		// do this twice so we can correct the position on both axis
		for (int n = 0; n < 2; n++) {
			// get a list of tiles traversed by the player
			std::vector<Tile> path;
			map.tile_path(path, c1, c2);

			if (map.tile_has_obstacle(path[0])) {
				fprintf(stderr, "OMG already in an obstacle!\n");
				// special case where a player is already inside an obstacle
				// from the starting position
			}

			// inspect the path
			for (unsigned int i = 1; i < path.size(); i++) {

				if (!map.tile_has_obstacle(path[i])) {
					// no obstacle here, check next tile
					continue;
				}

				// obstacle found!
				// mvtx and mvty are garanteed to be either 1, 0 or -1
				int mvtx = path[i].x - path[i-1].x;
				int mvty = path[i].y - path[i-1].y;

				if (mvtx && mvty) {
					fprintf(stderr, "Tile path has both mvtx and mvty!\n");
					break;
				}

				else if (mvtx) {
					// obstacle is on the same row. correct position so we don't
					// traverse it.
					c2.x = (mvtx > 0)
						?  path[i].x   *MAPUSIZE-EPSILON
						: (path[i].x+1)*MAPUSIZE+EPSILON;
					break;
				}

				else if (mvty) {
					// this time, it's on the same column
					c2.y = (mvty > 0)
						?  path[i].y   *MAPUSIZE-EPSILON
						: (path[i].y+1)*MAPUSIZE+EPSILON;
					break;
				}
			}
		}

		// finaly, correct the position of the player
		p->set_pos(c2.x, c2.y);
	}
}

