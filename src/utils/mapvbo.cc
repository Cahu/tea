#include <GL/glew.h>

#include "mapvbo.hh"


// 72 verts :s
float cube[] = {
	 // front
	-0.5,  0.5,  0.5,
	 0.5,  0.5,  0.5,
	 0.5, -0.5,  0.5,
	-0.5, -0.5,  0.5,
	 // back
	-0.5,  0.5, -0.5,
	 0.5,  0.5, -0.5,
	 0.5, -0.5, -0.5,
	-0.5, -0.5, -0.5,
	 // left
	-0.5,  0.5,  0.5,
	-0.5,  0.5, -0.5,
	-0.5, -0.5, -0.5,
	-0.5, -0.5,  0.5,
	 // right
	 0.5,  0.5,  0.5,
	 0.5,  0.5, -0.5,
	 0.5, -0.5, -0.5,
	 0.5, -0.5,  0.5,
	 // top
	-0.5,  0.5,  0.5,
	-0.5,  0.5, -0.5,
	 0.5,  0.5, -0.5,
	 0.5,  0.5,  0.5,
	 // bottom
	-0.5, -0.5,  0.5,
	-0.5, -0.5, -0.5,
	 0.5, -0.5, -0.5,
	 0.5, -0.5,  0.5
};


void map_to_VBO(const TEA::Map &map, structVBO &sVBO, unsigned int usize)
{
	std::vector<float> verts;
	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				case GLYPH_WALL:
					for (unsigned int k = 0; k < 72; k += 3) {
						verts.push_back((cube[k+0]+j)*usize);
						verts.push_back((cube[k+1]+i)*usize);
						verts.push_back((cube[k+2]+0)*usize);
					}
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
		verts.size() * sizeof (float),
		&verts[0],
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
