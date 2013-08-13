#include <GL/glew.h>
#include <SDL/SDL.h>

#include <glm/glm.hpp> // math lib
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shaders.hh"
#include "utils/mapvbo.hh"

using TEA::Map;
using namespace glm;

// projection and aspect ratio
mat4 model;
mat4 view;
mat4 proj;
mat4 MVP;

vec3 base_eye(0.0, -20.0, 10.0);


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
		exit(EXIT_FAILURE);
	} else {
		view = lookAt(
			base_eye, // eye
			vec3( 0.0f,   0.0f,   0.0f), // center
			vec3( 0.0f,   1.0f,   0.0f)  // up
		);

		proj = perspective(45.0f, 1.0f*WIDTH/HEIGHT, 0.1f, 100.0f);

		model  = translate(mat4(1.0f), vec3(0.f, 0.f, 0.f));

		MVP = proj * view * model;
	}

}


void init_world(const char *file)
{
	map.load(file);
	map_to_VBO(map, mapvbo);
}


void draw_scene(float relx, float rely)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	view = lookAt(
		vec3( base_eye + vec3(relx, rely, 0.0)), // eye
		vec3(            vec3(relx, rely, 0.0)), // center
		vec3(            vec3(0.0,   1.0, 0.0))  // up
	);
	MVP = proj * view * model;

	// draw map
	glUseProgram(default_shader);

	GLint MVP_loc = glGetUniformLocation(default_shader, "MVP");
	glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, value_ptr(MVP));

	glBindBuffer(GL_ARRAY_BUFFER, mapvbo.buff);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_QUADS, 0, mapvbo.size);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);

	SDL_GL_SwapBuffers();
}
