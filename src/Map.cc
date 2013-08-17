#include "Map.hh"
#include "utils/splitstr.hh"

#include <cmath>
#include <cstdio>
#include <fstream>

#ifndef NDEBUG
	#include <assert.h>
#endif

#define MAX(x, y) ((x) > (y) ? (x) : (y))


namespace TEA {

	Map::Map()
	{
		_height = 0;
		_width = 0;
	}


	Map::Map(const char *file)
	{
		_height = 0;
		_width = 0;
		load(file);
	}


	int Map::load(const char *file)
	{
		std::fstream f(file, std::fstream::in);

		if (!f.is_open()) {
			return -1;
		}

		_width = 0;
		_height = 0;
		_map.clear();
		_obstacles.clear();

		_height++;
		_map.push_back(std::vector<char>());

		char c;
		unsigned int x = 0;
		while (EOF != (c = f.get())) {
			x++;

			switch (c) {
				case '\n':
					_height++;
					_width = MAX(_width, x);
					_map.push_back(std::vector<char>()); // add row
					x = 0;
					break;
				case GLYPH_WALL:
					_obstacles.push_back(
						Coor(MAPUSIZE*(x-1), MAPUSIZE*(_height-1))
					);
				case GLYPH_EMPTY:
				default:
					_map.back().push_back(c);
					break;
			}
		}

		f.close();

		// make all lines the same length
		for (unsigned int i = 0; i < _map.size(); i++) {
#ifndef NDEBUG
			assert(_map[i].size() <= _width);
#endif
			_map[i].resize(_width, GLYPH_EMPTY);
		}

		return 0;
	}


	void Map::print() const
	{
		for (unsigned int i = 0; i < _map.size(); i++) {
			for (unsigned int j = 0; j < _map[i].size(); j++) {
				putchar(_map[i][j]);
			}
			puts("");
		}
	}


	const std::vector< std::vector<char> > &Map::data() const
	{
		return _map;
	}


	const std::vector< Coor > &Map::get_obstacles() const
	{
		return _obstacles;
	}


	unsigned int Map::get_width() const
	{
		return _width;
	}


	unsigned int Map::get_height() const
	{
		return _height;
	}


	bool Map::tile_has_obstacle(unsigned int x, unsigned int y) const
	{
		if (y >= _map.size()) {
			return false;
		}

		if (x >= _map[y].size()) {
			return false;
		}

		return _map[y][x] != GLYPH_EMPTY;
	}


	bool Map::on_same_row(const Coor &a, const Coor &b) const
	{
		return (floor(a.y) == floor(b.y));
	}


	bool Map::on_same_col(const Coor &a, const Coor &b) const
	{
		return (floor(a.x) == floor(b.x));
	}


	bool Map::on_same_tile(const Coor &a, const Coor &b) const
	{
		return on_same_col(a, b) && on_same_row(a, b);
	}


	void Map::tile_path(
		std::vector<Coor> &dst,
		const Coor &s,
		const Coor &e
	) const
	{
		double x = s.x;
		double y = s.y;

		// The way this works:
		// Imagine the path from 's' to 'e' can be ?traveled? in 1
		// unit of time. This gives us a 'speed' on both axis:
		double xspeed = e.x - s.x;
		double yspeed = e.y - s.y;

		// find out how many loops are necessary:
		unsigned int n;
		n  = (unsigned int) ceil(std::abs(e.x - s.x) / MAPUSIZE);
		n += (unsigned int) ceil(std::abs(e.y - s.y) / MAPUSIZE);

		while (n--) {
			dst.push_back(Coor(floor(x/MAPUSIZE), floor(y/MAPUSIZE)));

			// if both starting point and destination point are on the same
			// tile, there's little to do:
			if (on_same_tile(Coor(x, y), e)) {
				break;
			}

			// special case: pure vertical or horizontal paths
			if (xspeed == 0) {
				y += (yspeed > 0) ? MAPUSIZE : -MAPUSIZE;
				continue;
			} else if (yspeed == 0) {
				x += (xspeed > 0) ? MAPUSIZE : -MAPUSIZE;
				continue;
			}

			// Let's get the starting coordinates of next tiles in both
			// directions.  Note that we have to use floor or ceil depending
			// on the movement direction.
			// When going towards -x or -y, we need to substract 1 to the
			// number because tiles boudaries are not shared. A tile owns its
			// top and left boundaries, the right and bottom ones belong to
			// neighbouring tiles.
			double nxt_col = (xspeed > 0)
				? floor((x + MAPUSIZE) / MAPUSIZE) * MAPUSIZE
				: ceil ((x - MAPUSIZE) / MAPUSIZE) * MAPUSIZE - 1;
			double nxt_row = (yspeed > 0)
				? floor((y + MAPUSIZE) / MAPUSIZE) * MAPUSIZE
				: ceil ((y - MAPUSIZE) / MAPUSIZE) * MAPUSIZE - 1;

			// Now we can find if the next tile we encounter is on the x axis
			// or on the y axis based on the starting position and speeds on
			// both axis:
			double time_nxt_col = (nxt_col - x) / xspeed;
			double time_nxt_row = (nxt_row - y) / yspeed;

			// save these for later
			double oldx = x;
			double oldy = y;

			// update x and y
			if (time_nxt_col <= time_nxt_row) {
				x = nxt_col;
				y += time_nxt_col * yspeed;
			} else {
				x += time_nxt_row * xspeed;
				y = nxt_row;
			}

			if (
				// special case: went right through the corner, don't allow
				// that by adding a tile as transition. We choose to add the
				// one on the same column.
				   (0 != on_same_col(Coor(oldx, oldy), Coor(x, y)))
				&& (0 != on_same_row(Coor(oldx, oldy), Coor(x, y)))
			   ) {
				dst.push_back(Coor(floor(oldx/MAPUSIZE), floor(y/MAPUSIZE)));
			}
		}
	}
}
