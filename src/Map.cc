#include "Map.hh"
#include "utils/splitstr.hh"

#include <cstdio>
#include <fstream>

#ifndef NDEBUG
	#include <assert.h>
#endif

#define MAX(x, y) ((x) > (y) ? (x) : (y))


namespace TEA {

	Map::Map()
	{
		_nobstacles = 0;
		_height = 0;
		_width = 0;
	}


	Map::Map(const char *file)
	{
		_nobstacles = 0;
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
		_nobstacles = 0;
		_map.clear();

		_height++;
		_map.push_back(std::vector<char>());

		char c;
		unsigned int tmp_width = 0;
		while (EOF != (c = f.get())) {
			tmp_width++;

			switch (c) {
				case '\n':
					_height++;
					_width = MAX(_width, tmp_width);
					_map.push_back(std::vector<char>());
					tmp_width = 0;
					break;
				case GLYPH_WALL:
					_nobstacles++;
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
}
