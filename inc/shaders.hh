#include <GL/glew.h>
#include <vector>
#include <string>


GLuint load_shader(GLenum, const char *);
GLuint make_program(const std::vector<GLuint> &);
