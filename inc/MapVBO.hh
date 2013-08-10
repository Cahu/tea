#include <GL/gl.h>

#include "Map.hh"

namespace TEA {

	class MapVBO: public Map {
		GLuint _buff;
		size_t _size;
		unsigned int _usize;

		private:
		void buildVBO();
		void destroyVBO();

		public:
		~MapVBO();
		MapVBO(unsigned int = 25);
		MapVBO(const char *, unsigned int = 25);

		void render();
		int load(const char *);
	};
}
