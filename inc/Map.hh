#include <vector>

#define GLYPH_EMPTY ' '
#define GLYPH_WALL  '#'


namespace TEA {

	class Map {
		protected:
		unsigned int _width;
		unsigned int _height;
		unsigned int _nobstacles;

		std::vector< std::vector<char> > _map;

		public:
		Map();
		Map(const char *file);

		void print() const;
		int load(const char *file);

		const std::vector< std::vector<char> > &data() const;
	};
}
