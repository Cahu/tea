#include <GL/glew.h>

#include "mapvbo.hh"
#include "geometry.hh"


void map_to_VBO(const TEA::Map &map, structVBO &sVBO)
{
	std::vector<float> verts;
	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				case GLYPH_WALL:
					for (unsigned int k = 0; k < CUBE_VERTS; k += 3) {
						verts.push_back((CUBE[k+0]+j));
						verts.push_back((CUBE[k+1]+i));
						verts.push_back((CUBE[k+2]+0));
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
