#include "shaders.hh"

#include <fstream>
#include <sstream>


GLuint load_shader(GLenum type, const char *file)
{
	std::ifstream f(file);
	std::stringstream ss;
	std::string code;

	if (!f.is_open()) {
		return 0;
	}

	GLuint shader = glCreateShader(type);

	// load shader code
	ss << f.rdbuf();
	code = ss.str();

	GLint len = code.size();
	const char *code_str = code.c_str();
	glShaderSource(shader, 1, &code_str, &len);

	// compile
	glCompileShader(shader);

	// check compilation
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		GLint infolength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolength);

		GLchar *info = new GLchar[infolength + 1];
		glGetShaderInfoLog(shader, infolength, NULL, info);

		fprintf(stderr, "Compile failure in shader %s:\n%s\n", file, info);

		delete[] info;

		glDeleteShader(shader);

		return 0;
	}

	return shader;
}


GLuint make_program(const std::vector<GLuint> &shaders)
{
	GLuint program = glCreateProgram();

	for (unsigned int i = 0; i < shaders.size(); i++) {
		glAttachShader(program, shaders[i]);
	}

	glLinkProgram(program);

	// check link status
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status != GL_TRUE) {
		GLint infolength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infolength);

		GLchar *info = new GLchar[infolength + 1];
		glGetProgramInfoLog(program, infolength, NULL, info);

		fprintf(stderr, "link failure: %s\n", info);

		delete[] info;
		glDeleteProgram(program);

		return 0;
	}

	for (unsigned int i = 0; i < shaders.size(); i++) {
		glDetachShader(program, shaders[i]);
	}

	return program;
}
