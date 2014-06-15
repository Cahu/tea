#include <cstdio>
#include <cstdlib>
#include "playerutils.hh"

using TEA::Map;
using TEA::Player;


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

		Coor c1(p->get_xpos(), p->get_ypos());
		p->tick(delay);
		Coor c2(p->get_xpos(), p->get_ypos());

		for (int n = 0; n < 2; n++) {
			// do this twice so we can correct the position on both axis
			std::vector<Tile> path;

			map.tile_path(path, c1, c2);

			printf(
				"path %d between [%2.2f:%2.2f] and [%2.2f:%2.2f]:\t",
				n, c1.x, c1.y, c2.x, c2.y
			);
			for (unsigned int i = 0; i < path.size(); i++) {
				printf(" [%d:%d]", path[i].x, path[i].y);
			}
			puts("");

			if (map.tile_has_obstacle(path[0])) {
				fprintf(stderr, "OMG already in an obstacle!\n");
				exit(EXIT_FAILURE);
				// special case where a player is already inside an obstacle
				// from the starting position
			}


			// inspect the path
			for (unsigned int i = 1; i < path.size(); i++) {

				if (!map.tile_has_obstacle(path[i])) {
					continue;
				}

				// mvtx and mvty are garanteed to be either 1, 0 or -1
				int mvtx = path[i].x - path[i-1].x;
				int mvty = path[i].y - path[i-1].y;

				if (mvtx && mvty) {
					fprintf(stderr, "Tile path has both mvtx and mvty!\n");
				}

				if (mvtx) {
					c2.x = (mvtx > 0)
						? path[i].x*MAPUSIZE-0.01
						: (path[i].x+1)*MAPUSIZE+0.01;
					break;
				}

				if (mvty) {
					c2.y = (mvty > 0)
						? path[i].y*MAPUSIZE-0.01
						: (path[i].y+1)*MAPUSIZE+0.01;
					break;
				}
			}
		}

		printf("result: [%f:%f]\n", c2.x, c2.y);
		p->set_pos(c2.x, c2.y);

		puts("===");
	}
}
