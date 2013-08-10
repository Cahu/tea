#include <GL/glew.h>
#include <cstdio>

#include "MapVBO.hh"


namespace TEA {

	MapVBO::MapVBO(unsigned int usize)
	{
		_size = 0;
		_buff = 0;
		_usize = usize;
	}


	MapVBO::MapVBO(const char *file, unsigned int usize)
		: Map(file)
	{
		_usize = usize;
		buildVBO();
	}


	MapVBO::~MapVBO()
	{
		destroyVBO();
	}


	int MapVBO::load(const char *file)
	{
		if (_buff != 0) {
			destroyVBO();
		}

		int rv = Map::load(file);

		if (rv < 0) {
			return rv;
		}

		buildVBO();

		return 0;
	}


	void MapVBO::buildVBO()
	{
		std::vector<float> verts;

		for (unsigned int i = 0; i < _map.size(); i++) {

			for (unsigned int j = 0; j < _map[i].size(); j++) {

				switch (_map[i][j]) {
					case GLYPH_WALL:
						verts.push_back(j*_usize);
						verts.push_back(i*_usize);
						verts.push_back(0.f);
						verts.push_back(1.f);
						//--
						verts.push_back((j+1)*_usize);
						verts.push_back(i*_usize);
						verts.push_back(0.f);
						verts.push_back(1.f);
						//--
						verts.push_back((j+1)*_usize);
						verts.push_back((i+1)*_usize);
						verts.push_back(0.f);
						verts.push_back(1.f);
						//--
						verts.push_back(j*_usize);
						verts.push_back((i+1)*_usize);
						verts.push_back(0.f);
						verts.push_back(1.f);
						break;
					default:
						break;
				}
			}
		}

		_size = verts.size();

		glGenBuffers(1, &_buff);
		glBindBuffer(GL_ARRAY_BUFFER, _buff);
		glBufferData(
			GL_ARRAY_BUFFER,
			sizeof(float) * verts.size(),
			&verts[0],
			GL_STATIC_DRAW
		);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	void MapVBO::render()
	{
		glBindBuffer(GL_ARRAY_BUFFER, _buff);
		glVertexPointer(4, GL_FLOAT, 0, NULL);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDrawArrays(GL_QUADS, 0, _size);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void MapVBO::destroyVBO()
	{
		glDeleteBuffers(1, &_buff);
		_size = 0;
		_buff = 0;
	}
}
