#ifndef _STRUCTBVO_
#define _STRUCTBVO_

#include <GL/gl.h>
#include <cstddef>


struct structVBO {
	GLuint verts;
	GLuint normals;
	size_t size;
	structVBO() : verts(0), normals(0), size(0) {};
};

#endif
