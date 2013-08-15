#include <GL/glew.h>

#include "mapvbo.hh"
#include "geometry.hh"

#define USIZE 2   // make walls two times the unit size


void map_to_VBO(const TEA::Map &map, structVBO &sVBO)
{
	std::vector<float> verts, normals;
	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				case GLYPH_WALL:
					for (unsigned int k = 0; k < 3*CUBE_N_VERTS; k += 3) {
						verts.push_back((CUBE_VERTS[k+0]+j)*USIZE);
						verts.push_back((CUBE_VERTS[k+1]+i)*USIZE);
						verts.push_back((CUBE_VERTS[k+2]+0)*USIZE);
						normals.push_back(CUBE_NORMALS[k+0]);
						normals.push_back(CUBE_NORMALS[k+1]);
						normals.push_back(CUBE_NORMALS[k+2]);
					}
					break;
				default:
					break;
			}
		}
	}

	sVBO.size = verts.size();

	glGenBuffers(1, &sVBO.verts);
	glGenBuffers(1, &sVBO.normals);

	glBindBuffer(GL_ARRAY_BUFFER, sVBO.verts);
	glBufferData(
		GL_ARRAY_BUFFER,
		verts.size() * sizeof (float),
		&verts[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, sVBO.normals);
	glBufferData(
		GL_ARRAY_BUFFER,
		normals.size() * sizeof (float),
		&normals[0],
		GL_STATIC_DRAW
	);
}
