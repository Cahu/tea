#include <GL/glew.h>
#include <SDL/SDL.h>

#include <glm/glm.hpp> // math lib
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Player.hh"
#include "shaders.hh"
#include "geometry.hh"
#include "utils/mapvbo.hh"

using TEA::Map;
using TEA::Player;
using namespace glm;

// projection and aspect ratio
mat4 model;
mat4 view;
mat4 proj;
mat4 MVP;

// predefined colors
vec4 RED  (1.0, 0.0, 0.0, 1.0);
vec4 BLUE (0.7, 0.7, 1.0, 1.0);
vec4 BLACK(0.05, 0.05, 0.1, 1.0);


// config variables
static unsigned int WIDTH  = 600;
static unsigned int HEIGHT = 400;

static Map map;
static structVBO mapvbo;
static structVBO floorvbo;
static structVBO playervbo;
static structVBO stencil;
static GLuint default_program;
static GLuint stencil_program;


void init_sdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Can't init sdl.\n");
		exit(EXIT_FAILURE);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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
	glClearStencil(0x0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glDepthFunc(GL_LESS);

	// VBOs for the map
	glGenBuffers(1, &mapvbo.verts);
	glGenBuffers(1, &mapvbo.normals);
	glGenBuffers(1, &floorvbo.verts);
	glGenBuffers(1, &floorvbo.normals);


	// prepare stencil shapes VBO
	stencil.size = 0;
	glGenBuffers(1, &stencil.verts);

	// build the player model vbo
	playervbo.size = DPYRAMID_N_VERTS;
	glGenBuffers(1, &playervbo.verts);
	glGenBuffers(1, &playervbo.normals);

	glBindBuffer(GL_ARRAY_BUFFER, playervbo.verts);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof DPYRAMID_VERTS,
		DPYRAMID_VERTS,
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, playervbo.normals);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof DPYRAMID_NORMALS,
		DPYRAMID_NORMALS,
		GL_STATIC_DRAW
	);


	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/** shaders **/
	GLuint vshader, fshader;
	std::vector<GLuint> shaders;

	// default shader
	shaders.clear();
	vshader = load_shader(GL_VERTEX_SHADER, "shaders/default.vert");
	if (vshader == 0) { exit(EXIT_FAILURE); }
	fshader = load_shader(GL_FRAGMENT_SHADER, "shaders/default.frag");
	if (fshader == 0) { exit(EXIT_FAILURE); }
	shaders.push_back(vshader);
	shaders.push_back(fshader);
	default_program = make_program(shaders);
	if (default_program == 0) { exit(EXIT_FAILURE); }
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	// stencil shader (minimum computation!)
	shaders.clear();
	vshader = load_shader(GL_VERTEX_SHADER, "shaders/stencil.vert");
	if (vshader == 0) { exit(EXIT_FAILURE); }
	fshader = load_shader(GL_FRAGMENT_SHADER, "shaders/stencil.frag");
	if (fshader == 0) { exit(EXIT_FAILURE); }
	shaders.push_back(vshader);
	shaders.push_back(fshader);
	stencil_program = make_program(shaders);
	if (stencil_program == 0) { exit(EXIT_FAILURE); }
	glDeleteShader(vshader);
	glDeleteShader(fshader);


	// set MVP matrix
	model = mat4(1.0f);
	view = lookAt(
		vec3( 0.0 ,  0.0f, 40.0f), // eye
		vec3( 0.0f,  0.0f,  0.0f), // center
		vec3( 0.0f,  1.0f,  0.0f)  // up
	);
	proj = perspective(45.0f, 1.0f*WIDTH/HEIGHT, 35.0f, 45.0f);
	MVP = proj * view * model;

}


void use_map(const Map &newmap)
{
	map = newmap;
	map_to_VBO(map, mapvbo, floorvbo);
}


/****** DRAWING MESS ******/

// these are used for all drawing functions
GLint MP_loc;
GLint MVP_loc;
GLint Color_loc;
vec3  rel, srel;

void draw_map();
void draw_floor();
void update_stencil_buff();
void draw_players(const std::vector<Player *> &);


void draw_scene(const std::vector<Player *> &players, float relx, float rely)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glLoadIdentity();

	// move everything relative to the followed point
	rel  = vec3(relx, rely, 0.0);
	// same thing but relative to screen coordinate system
	srel = vec3(relx,-rely, 0.0);

	// stencil
	glUseProgram(stencil_program);
	MVP_loc = glGetUniformLocation(stencil_program, "MVP");

	glEnableVertexAttribArray(0);

	update_stencil_buff();

	glDisableVertexAttribArray(0);


	// drawing
	glUseProgram(default_program);
	MP_loc    = glGetUniformLocation(default_program, "MP");
	MVP_loc   = glGetUniformLocation(default_program, "MVP");
	Color_loc = glGetUniformLocation(default_program, "Color");
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glDisable(GL_STENCIL_TEST);
	draw_map();

	glEnable(GL_STENCIL_TEST);
	draw_floor();
	draw_players(players);

	// cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	SDL_GL_SwapBuffers();
}


void draw_players(const std::vector<Player *> &players)
{
	glUniform4fv(Color_loc, 1, value_ptr(RED));

	glBindBuffer(GL_ARRAY_BUFFER, playervbo.verts);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, playervbo.normals);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	for (unsigned int i = 0; i < players.size(); i++) {
		Player *p = players[i];
		if (p == NULL) continue;

		float x = p->get_xpos();
		float y = p->get_ypos();

		// matrix transformation
		// minus y because axis is reversed when rendering!
		model = translate(mat4(1.0f), vec3(x, -y, 0.f) - srel);
		MVP = proj * view * model;
		glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, value_ptr(MVP));
		glUniformMatrix4fv(MP_loc, 1, GL_FALSE, value_ptr(model));

		// draw
		glDrawArrays(GL_TRIANGLES, 0, playervbo.size);
	}
}


void draw_map()
{
	// move the map arround the player
	// map y axis is reversed in respect to the screen y axis
	model  = translate(mat4(1.0f), -srel);
	model *= scale(mat4(1.0f), vec3(1.0, -1.0, 1.0));

	MVP = proj * view * model;
	glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, value_ptr(MVP));
	glUniformMatrix4fv(MP_loc, 1, GL_FALSE, value_ptr(model));

	glUniform4fv(Color_loc, 1, value_ptr(BLUE));

	glBindBuffer(GL_ARRAY_BUFFER, mapvbo.verts);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, mapvbo.normals);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, mapvbo.size);
}


void draw_floor()
{
	model  = translate(mat4(1.0f), -srel);
	model *= scale(mat4(1.0f), vec3(1.0, -1.0, 1.0));

	MVP = proj * view * model;
	glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, value_ptr(MVP));
	glUniformMatrix4fv(MP_loc, 1, GL_FALSE, value_ptr(model));

	glUniform4fv(Color_loc, 1, value_ptr(BLACK));

	glBindBuffer(GL_ARRAY_BUFFER, floorvbo.verts);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, floorvbo.normals);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, floorvbo.size);
}


void vector_append_vec(std::vector<float> &dst, vec3 vec)
{
	dst.push_back(vec.x);
	dst.push_back(vec.y);
	dst.push_back(vec.z);
}


void proj_face(
	std::vector<float> &dst,
	vec3 corner1,
	vec3 corner2,
	double radius
) {
	vec3 uni;

	// Cast the basic shadow
	uni = normalize(corner1 - rel);
	vec3 proj1 = rel + uni * vec3(radius, radius, radius);
	uni = normalize(corner2 - rel);
	vec3 proj2 = rel + uni * vec3(radius, radius, radius);

	// append two tris equivalent to a quad for the shadow
	vector_append_vec(dst, corner1);
	vector_append_vec(dst, proj1  );
	vector_append_vec(dst, proj2  );
	vector_append_vec(dst, corner1);
	vector_append_vec(dst, proj2  );
	vector_append_vec(dst, corner2);

	// get perpendicular vector of proj1 --> proj2 to create a rectangle that
	// will hide holes in some particular cases (when the slope is too high).
	// For instance:
	// +------------/|------------+
	// |           / |            |
	// |          /  |            |
	// |         +----+   we have |
	// |         |   ||   a hole  | -> fill the hole with a rectangle
	// |       o |   ||   here    |    using a perpendicular vector to
	// |         +----+           |    the farthest side of the shadow
	// |          \  |            |
	// +-----------\-|------------+
	vec3 proj12 = proj2 - proj1;
	vec3 comp = normalize(vec3(-proj12.y, proj12.x, proj12.z));

	vector_append_vec(dst, proj1);
	vector_append_vec(dst, proj1 - comp * vec3(radius, radius, radius));
	vector_append_vec(dst, proj2 - comp * vec3(radius, radius, radius));

	vector_append_vec(dst, proj1);
	vector_append_vec(dst, proj2 - comp * vec3(radius, radius, radius));
	vector_append_vec(dst, proj2);
}

void update_stencil_buff()
{
	const std::vector<Tile> &obs = map.get_obstacles();

	// nothing to do
	if (obs.size() == 0) {
		return;
	}

	// use a sphere big enough to cover the screen to cast projections on it
	double proj_length = map.get_width() + map.get_height();

	// draw shadows of:
	// at most 2 sides of the obstacle,
	// 2 shadow parts per side,
	// with 6 verts each,
	size_t prediction = obs.size() * 6 * 2 * 2;

	// realloc if insuficient space
	if (stencil.size < prediction) {
		fprintf(stderr, "INFO: reallocating VBO for stencil buffer\n");
		if (stencil.size == 0) {
			stencil.size = prediction;
		} else {
			stencil.size *= 2; // exponential growth
		}

		glBindBuffer(GL_ARRAY_BUFFER, stencil.verts);
		glBufferData(
			GL_ARRAY_BUFFER,
			stencil.size * 3 * sizeof(float), // 3 coors per vertex
			NULL,
			GL_STREAM_DRAW
		);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// cast shadows
	std::vector< float > shadows_verts;
	shadows_verts.reserve(prediction*3);
	for (unsigned int i = 0; i < obs.size(); i++) {
		// obstacle center
		Coor c(obs[i]);

		// make sure to pass corners in the right order to proj_face
		// (important for the completion rectangle trick)
		if (rel.x < c.x) {
			// we can see the left side of the obstacle
			vec3 tl_corner = vec3(c.x, c.y         , 0.0);
			vec3 bl_corner = vec3(c.x, c.y+MAPUSIZE, 0.0);
			proj_face(shadows_verts, tl_corner, bl_corner, proj_length);
		}
		else if (rel.x > c.x + MAPUSIZE) {
			// we can see the right side of the obstacle
			vec3 tr_corner = vec3(c.x+MAPUSIZE, c.y         , 0.0);
			vec3 br_corner = vec3(c.x+MAPUSIZE, c.y+MAPUSIZE, 0.0);
			proj_face(shadows_verts, br_corner, tr_corner, proj_length);
		}

		if (rel.y < c.y) {
			// we can see the top side of the obstacle
			vec3 tl_corner = vec3(c.x         , c.y, 0.0);
			vec3 tr_corner = vec3(c.x+MAPUSIZE, c.y, 0.0);
			proj_face(shadows_verts, tr_corner, tl_corner, proj_length);
		}
		else if (rel.y > c.y + MAPUSIZE) {
			// we can see the bottom side of the obstacle
			vec3 bl_corner = vec3(c.x         , c.y+MAPUSIZE, 0.0);
			vec3 br_corner = vec3(c.x+MAPUSIZE, c.y+MAPUSIZE, 0.0);
			proj_face(shadows_verts, bl_corner, br_corner, proj_length);
		}
	}

	// update stencil buffer content
#ifndef NDEBUG
	assert(stencil.size >= shadows_verts.size()/3);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, stencil.verts);
	glBufferSubData(
		GL_ARRAY_BUFFER,
		0,
		shadows_verts.size() * sizeof(float),
		&shadows_verts[0]
	);

	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	model  = translate(mat4(1.0f), -srel);
	model *= scale(mat4(1.0f), vec3(1.0, -1.0, 1.0));
	MVP = proj * view * model;
	glUniformMatrix4fv(MVP_loc, 1, GL_FALSE, value_ptr(MVP));

	glBindBuffer(GL_ARRAY_BUFFER, stencil.verts);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glDrawArrays(GL_TRIANGLES, 0, stencil.size);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}
