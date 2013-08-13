#include <GL/glew.h>
#include <SDL/SDL.h>

#include <glm/glm.hpp> // math lib
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shaders.hh"
#include "utils/mapvbo.hh"

using TEA::Map;

// projection and aspect ratio
glm::mat4 model;
glm::mat4 view;
glm::mat4 proj;
glm::mat4 MVP;

// config variables
static unsigned int WIDTH  = 800;
static unsigned int HEIGHT = 600;

static Map map;
static structVBO mapvbo;
static GLuint default_shader;


void init_sdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Can't init sdl.\n");
		exit(EXIT_FAILURE);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// TODO: cleanup the object returned by SetVideoMode
	if (!SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_OPENGL)) {
		fprintf(stderr, "Can't set video mode.\n");
		exit(EXIT_FAILURE);
	}
}


void init_opengl(void)
{
	// gl extensions
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr,
			"GLEW failed to initialize: %s\n",
			glewGetErrorString(err)
		);
		exit(EXIT_FAILURE);
	}

	// default stuff
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);

	// shaders stuff
	std::vector<GLuint> shaders;

	GLuint vshader = load_shader(GL_VERTEX_SHADER, "shaders/default.vert");
	if (vshader == 0) { exit(EXIT_FAILURE); }

	GLuint fshader = load_shader(GL_FRAGMENT_SHADER, "shaders/default.frag");
	if (fshader == 0) { exit(EXIT_FAILURE); }

	shaders.push_back(vshader);
	shaders.push_back(fshader);
	default_shader = make_program(shaders);
	if (default_shader == 0) { exit(EXIT_FAILURE); }
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	// set projection matrix
	GLint MVP_loc = glGetUniformLocation(default_shader, "MVP");
	if (-1 == MVP_loc) {
		fprintf(stderr, "Can't set MVP matrix\n");
		//exit(EXIT_FAILURE);
	} else {
		view = glm::lookAt(
			glm::vec3( 0.0f, -5.0f,  5.0f), // eye
			glm::vec3( 0.0f,  0.0f,  0.0f), // center
			glm::vec3( 0.0f,  1.0f,  0.0f)  // up
		);

		proj = glm::perspective(45.0f, 1.0f*WIDTH/HEIGHT, 0.1f, 100.0f);

		model  = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f));
		//model *= glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));

		MVP = proj * view * model;
	}

	glUseProgram(default_shader);
	glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, glm::value_ptr(MVP));
}


void init_world(const char *file)
{
	map.load(file);
	map_to_VBO(map, mapvbo);
}


void draw_scene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// draw map
	glBindBuffer(GL_ARRAY_BUFFER, mapvbo.buff);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_QUADS, 0, mapvbo.size);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);

	SDL_GL_SwapBuffers();
}
