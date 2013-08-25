#include <GL/glew.h>

#include "mapvbo.hh"
#include "geometry.hh"


void map_to_VBO(const TEA::Map &map, structVBO &walls, structVBO &floor)
{
	std::vector<float> m_verts, m_normals;
	std::vector<float> f_verts, f_normals;

	const std::vector<std::vector<char> > &mapdata = map.data();

	for (unsigned int i = 0; i < mapdata.size(); i++) {

		for (unsigned int j = 0; j < mapdata[i].size(); j++) {

			switch (mapdata[i][j]) {
				// All basic shapes are centered on 0,0 and are of size 1
				// unit. We need to apply +0.5 in order to place the top left
				// corner of every shape at 0,0 and scale with MAPUSIZE.
				case GLYPH_WALL:
					for (unsigned int k = 0; k < 3*CUBE_N_VERTS; k += 3) {
						m_verts.push_back((CUBE_VERTS[k+0]+j+0.5)*MAPUSIZE);
						m_verts.push_back((CUBE_VERTS[k+1]+i+0.5)*MAPUSIZE);
						m_verts.push_back((CUBE_VERTS[k+2])*MAPUSIZE);
						m_normals.push_back(CUBE_NORMALS[k+0]);
						m_normals.push_back(CUBE_NORMALS[k+1]);
						m_normals.push_back(CUBE_NORMALS[k+2]);
					}
					break;
				case GLYPH_EMPTY:
					for (unsigned int k = 0; k < 3*TILE_N_VERTS; k += 3) {
						f_verts.push_back((TILE_VERTS[k+0]+j+0.5)*MAPUSIZE);
						f_verts.push_back((TILE_VERTS[k+1]+i+0.5)*MAPUSIZE);
						f_verts.push_back((TILE_VERTS[k+2]-0.5)*MAPUSIZE);
						f_normals.push_back(TILE_NORMALS[k+0]);
						f_normals.push_back(TILE_NORMALS[k+1]);
						f_normals.push_back(TILE_NORMALS[k+2]);
					}
					break;
				default:
					break;
			}
		}
	}

	walls.size = m_verts.size();
	floor.size = f_verts.size();

	glBindBuffer(GL_ARRAY_BUFFER, walls.verts);
	glBufferData(
		GL_ARRAY_BUFFER,
		m_verts.size() * sizeof (float),
		&m_verts[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, walls.normals);
	glBufferData(
		GL_ARRAY_BUFFER,
		m_normals.size() * sizeof (float),
		&m_normals[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, floor.verts);
	glBufferData(
		GL_ARRAY_BUFFER,
		f_verts.size() * sizeof (float),
		&f_verts[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, floor.normals);
	glBufferData(
		GL_ARRAY_BUFFER,
		f_normals.size() * sizeof (float),
		&f_normals[0],
		GL_STATIC_DRAW
	);
}
