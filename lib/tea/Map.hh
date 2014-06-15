#ifndef _CLASS_MAP_
#define _CLASS_MAP_

#include <vector>

#define GLYPH_EMPTY ' '
#define GLYPH_WALL  '#'

#define MAPUSIZE 2   // make walls two times the unit size

struct Tile;
struct Coor;

struct Coor {
	double x;
	double y;
	Coor(double, double);
	Coor(const Tile &);
};

struct Tile {
	int x;
	int y;
	Tile(int, int);
	Tile(const Coor &);
};


namespace TEA {

	class Map {
		unsigned int _width;
		unsigned int _height;

		std::vector< Tile > _obstacles;
		std::vector< std::vector<char> > _map;

		public:
		Map();
		Map(const char *file);

		void print() const;
		int load(const char *file);

		const std::vector< Tile > &get_obstacles() const;
		const std::vector< std::vector<char> > &data() const;

		unsigned int get_width() const;
		unsigned int get_height() const;

		bool tile_has_obstacle(const Tile &) const;

		bool on_same_row(const Tile &, const Tile &) const;
		bool on_same_col(const Tile &, const Tile &) const;
		bool on_same_tile(const Tile &, const Tile &) const;

		void tile_path(std::vector<Tile> &, const Coor &, const Coor &) const;
	};
}

#endif
