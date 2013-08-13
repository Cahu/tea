#include <GL/glew.h>

#include "mapvbo.hh"


void map_to_VBO(const TEA::Map &map, structVBO &sVBO, unsigned int usize)
{
	std::vector<float> verts;
	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				case GLYPH_WALL:
					verts.push_back(+1.0*j*usize);
					verts.push_back(-1.0*i*usize);
					verts.push_back(0.0);
					//--
					verts.push_back(+1.0*(j+1)*usize);
					verts.push_back(-1.0*i*usize);
					verts.push_back(0.0);
					//--
					verts.push_back(+1.0*(j+1)*usize);
					verts.push_back(-1.0*(i+1)*usize);
					verts.push_back(0.0);
					//--
					verts.push_back(+1.0*j*usize);
					verts.push_back(-1.0*(i+1)*usize);
					verts.push_back(0.0);
					break;
				default:
					break;
			}
		}
	}

	/*
	float vertices[] = {
		-0.5, -0.5, 0.0,
		-0.5,  0.5, 0.0,
		 0.5,  0.5, 0.0,
		 0.5, -0.5, 0.0
	};
	*/

	sVBO.size = verts.size();

	glGenBuffers(1, &sVBO.buff);
	glBindBuffer(GL_ARRAY_BUFFER, sVBO.buff);
	glBufferData(
		GL_ARRAY_BUFFER,
		verts.size() * sizeof (float),
		&verts[0],
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
