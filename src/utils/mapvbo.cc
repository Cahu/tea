#include <GL/glew.h>

#include "mapvbo.hh"


void map_to_VBO(const TEA::Map &map, structVBO &sVBO, float usize)
{
	std::vector<float> verts;
	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				case GLYPH_WALL:
					verts.push_back(j*usize);
					verts.push_back(i*usize);
					//--
					verts.push_back((j+1)*usize);
					verts.push_back(i*usize);
					//--
					verts.push_back((j+1)*usize);
					verts.push_back((i+1)*usize);
					//--
					verts.push_back(j*usize);
					verts.push_back((i+1)*usize);
					break;
				default:
					break;
			}
		}
	}

	sVBO.size = verts.size();

	glGenBuffers(1, &sVBO.buff);
	glBindBuffer(GL_ARRAY_BUFFER, sVBO.buff);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(float) * verts.size(),
		&verts[0],
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
