#ifndef _CLASS_MAP_
#define _CLASS_MAP_

#include <vector>

#define GLYPH_EMPTY ' '
#define GLYPH_WALL  '#'

#define MAPUSIZE 2   // make walls two times the unit size

struct Coor {
	double x;
	double y;
	Coor(double xx, double yy) : x(xx), y(yy) {};
};


namespace TEA {

	class Map {
		unsigned int _width;
		unsigned int _height;

		std::vector< Coor > _obstacles;
		std::vector< std::vector<char> > _map;

		public:
		Map();
		Map(const char *file);

		void print() const;
		int load(const char *file);

		const std::vector< Coor > &get_obstacles() const;
		const std::vector< std::vector<char> > &data() const;

		unsigned int get_width() const;
		unsigned int get_height() const;

		bool tile_has_obstacle(unsigned int, unsigned int) const;

		bool on_same_row(const Coor &, const Coor &) const;
		bool on_same_col(const Coor &, const Coor &) const;
		bool on_same_tile(const Coor &, const Coor &) const;

		void tile_path(std::vector<Coor> &, const Coor &, const Coor &) const;
	};
}

#endif
