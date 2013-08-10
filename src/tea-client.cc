#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>

#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef NDEBUG
	#include <assert.h>
#endif

#include <GL/glew.h>
#include <SDL/SDL.h>

#include "cmds.hh"
#include "keys.hh"
#include "MapVBO.hh"
#include "Player.hh"
#include "shaders.hh"
#include "utils/netutils.hh"
#include "utils/splitstr.hh"

#define PORT 9999
#define MAX_MSG_LEN 64

#define CHK(msg, cmd)       \
	if (0 > (cmd)) {        \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	}


using TEA::MapVBO;
using TEA::Player;

typedef short flag_t;



// config variables
unsigned int WIDTH  = 800;
unsigned int HEIGHT = 600;

// state variables
unsigned int id;
char playing;
flag_t flags;

// network stuff
static int tcp_sock;
static int udp_sock;

// objects collections
MapVBO map;
static std::vector<Player *> players;




// graphic functions
static void init_sdl();
static void init_opengl();
static void draw_scene();

// local events
static int handle_sdl_events(flag_t *);

// network function
static void init_connect(const char *, unsigned short);
static int handle_handshake(void);
static int handle_tcp_msg();
static int handle_udp_msg();
static int send_flags(flag_t);

// objects management
static void add_player(unsigned int pidx, double x = 0, double y = 0);
static void remove_player(unsigned int);




int main(int argc, char *argv[])
{
	init_sdl();
	init_opengl();

	int exitval = EXIT_SUCCESS;

	// init state variables
	flags = 0;
	playing = 0;

	map.load("map.txt");
	map.print();

	if (argc < 2) {
		init_connect("127.0.0.1", PORT);
	} else {
		init_connect(argv[1], PORT);
	}

	id = handle_handshake();
	if (id < 0) {
		fprintf(stderr, "Error during handshake\n");
		exitval = EXIT_FAILURE;
		goto END;
	}

	printf("got ID #%d\n", id);

	// init structures for poll()
	pollfd fds[2];
	fds[0].fd     = tcp_sock;
	fds[0].events = POLLIN;
	fds[1].fd     = udp_sock;
	fds[1].events = POLLIN;

	// main loop
	while (1) {

		// handle events on sockets
		while (poll(fds, 2, 0) > 0) {

			if (fds[0].revents & POLLIN) {
				if (-1 == handle_tcp_msg()) {
					fprintf(stderr, "Connexion with server lost\n");
					goto END;
				}
			}

			if (fds[1].revents & POLLIN) {
				if (-1 == handle_udp_msg()) {
					fprintf(stderr, "Connexion with server lost\n");
					goto END;
				}
			}
		}

		// events on mouse/keyboard
		if (handle_sdl_events(&flags)) {
			if (flags == -1) {
				goto END;
			}

			if (playing) {
				send_flags(flags);
				players[id]->set_flags(flags);
			}
		}

		// update state
		for (unsigned int i = 0; i < players.size(); i++) {
			Player *p = players[i];
			if (p != NULL) {
				p->tick(10);
			}
		}

		// draw
		draw_scene();

		// sleep
		SDL_Delay(10);
	}

	END:
	close(tcp_sock);
	close(udp_sock);

	exit(exitval);
}


void init_sdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Can't init sdl.\n");
		exit(EXIT_FAILURE);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	if (SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_OPENGL) < 0) {
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
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WIDTH, HEIGHT, 0, 1, -1);

	glMatrixMode(GL_MODELVIEW);

	// shaders stuff
	std::vector<GLuint> shaders;
	shaders.push_back(load_shader(GL_VERTEX_SHADER, "shaders/default.vert"));
	shaders.push_back(load_shader(GL_FRAGMENT_SHADER, "shaders/default.frag"));
	make_program(shaders);
}


void draw_scene(void)
{
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3f(1, 1, 1);
	glLoadIdentity();

	map.render();

	for (unsigned int i = 0; i < players.size(); i++) {
		Player *p = players[i];

		if (p != NULL) {
			glLoadIdentity();
			glTranslatef(p->get_xpos(), p->get_ypos(), 0);

			glBegin(GL_QUADS);
				glVertex3f( 0,  0, 0);
				glVertex3f(10,  0, 0);
				glVertex3f(10, 10, 0);
				glVertex3f( 0, 10, 0);
			glEnd();
		}
	}

	SDL_GL_SwapBuffers();
}


void init_connect(const char *addr, unsigned short port)
{
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(port);
	inet_pton(AF_INET, addr, &saddr.sin_addr.s_addr);

	CHK("socket", udp_sock = socket(AF_INET, SOCK_DGRAM, 0));
	CHK("connect", connect(udp_sock, (sockaddr *) &saddr, sizeof saddr));

	CHK("socket", tcp_sock = socket(AF_INET, SOCK_STREAM, 0));
	CHK("connect", connect(tcp_sock, (sockaddr *) &saddr, sizeof saddr));
}


void add_player(unsigned int pid, double x, double y)
{
	if (pid >= players.size()) {
		players.resize(pid+1, NULL);
	}

#ifndef NDEBUG
	assert(players[pid] == NULL);
#endif

	players[pid] = new Player(x, y);

	// server just added us as a player
	if (pid == id) {
		playing = 1;
	}
}


void remove_player(unsigned int pid)
{
	if (pid >= players.size() || players[pid] == NULL) {
		fprintf(stderr, "Trying to remove a player we don't know about!\n");
		return;
	}

	delete players[pid];
	players[pid] = NULL;

	// server just removed us from the game
	if (pid == id) {
		playing = 0;
	}
}


int handle_handshake(void)
{
	int id = -1;

	ssize_t size;
	char msg[MAX_MSG_LEN];

	// get a cookie
	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		fprintf(stderr, "Can't get cookie\n");
		return -1;
	}
	msg[size] = '\0';

	// TODO: resend if packet lost
	// send the cookie back (on udp socket!)
	unsigned int cookienum;
	if (1 == sscanf(msg, "COOKIE %u", &cookienum)) {
		size = sprintf(msg, "COOKIE %u", cookienum);
		fprintf(stderr, "got cookie %u\n", cookienum);
		send(udp_sock, msg, size, 0);
	}

	// get an ID
	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		fprintf(stderr, "Can't get an ID\n");
		return -1;
	}
	msg[size] = '\0';

	if (1 != sscanf(msg, "ID %d", &id)) {
		return -1;
	}

	return id;
}


int handle_sdl_events(flag_t *flags)
{
	SDL_Event event;
	int got_event = 0;

	while (SDL_PollEvent(&event)) {
		got_event = 1;

		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
				case SDLK_j:
					tcp_send(tcp_sock, CMD_JOIN, sizeof CMD_JOIN, 0);
					break;
				case SDLK_l:
					tcp_send(tcp_sock, CMD_LEAVE, sizeof CMD_LEAVE, 0);
					break;
				case SDLK_w:
					*flags |= KEY_UP;
					break;
				case SDLK_s:
					*flags |= KEY_DOWN;
					break;
				case SDLK_a:
					*flags |= KEY_LEFT;
					break;
				case SDLK_d:
					*flags |= KEY_RIGHT;
					break;
				default:
					break;
			}
		}

		else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
				case SDLK_w:
					*flags ^= KEY_UP;
					break;
				case SDLK_s:
					*flags ^= KEY_DOWN;
					break;
				case SDLK_a:
					*flags ^= KEY_LEFT;
					break;
				case SDLK_d:
					*flags ^= KEY_RIGHT;
					break;
				default:
					break;
			}
		}

		else if (event.type == SDL_QUIT) {
			tcp_send(tcp_sock, CMD_QUIT, sizeof CMD_QUIT, 0);
			return -1;
		}
	}

	return got_event;
}


int handle_tcp_msg(void)
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	unsigned int id;

	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size <= 0) {
		return -1;
	}
	msg[size] = '\0';

	if (1 == sscanf(msg, CMD_JOIN " %u", &id)) {
		add_player(id);
	}

	else if (1 == sscanf(msg, CMD_LEAVE " %u", &id)) {
		remove_player(id);
	}

	else if (strstr(msg, CMD_PLIST) == msg) {
		puts(msg);
		std::vector<std::string> items;
		splitstr(msg+sizeof(CMD_PLIST), items, ';');

		std::vector<std::string>::iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			double x, y;
			unsigned int pid;

			if (3 == sscanf(it->c_str(), "%u:%lf:%lf", &pid, &x, &y)) {
				add_player(pid, x, y);
			}
		}
	}

	else {
		fprintf(stderr, "Can't make sence of msg from server: %s\n", msg);
	}

	return 0;
}


int handle_udp_msg(void)
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	unsigned int pid;
	unsigned int pflags;

	size = recv(udp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size <= 0) {
		return -1;
	}
	msg[size] = '\0';

	if (2 == sscanf(msg, CMD_FLAGS "%u:%u", &pid, &pflags)) {
		if (pid == id) {
			; // TODO: use this as an ACK of previously sent flags
		} else {
			printf("got flags for player #%u: %u\n", pid, pflags);
			if (pid >= players.size() || players[pid] == NULL) {
				fprintf(stderr, "Got flags for player we don't know.\n");
			} else {
				players[pid]->set_flags(pflags);
			}
		}
	}

	else if (strstr(msg, CMD_SYNC) == msg) {
		puts(msg);
		std::vector<std::string> items;
		splitstr(msg+sizeof(CMD_SYNC), items, ';');

		std::vector<std::string>::iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			double x, y;
			unsigned int pid;

			if (3 == sscanf(it->c_str(), "%u:%lf:%lf", &pid, &x, &y)) {
				if (pid >= players.size() || players[pid] == NULL) {
					fprintf(stderr, "Got pos for player we don't know.\n");
				} else {
					players[pid]->set_pos(x, y);
				}
			}
		}
	}

	else {
		fprintf(stderr, "Can't make sence of msg from server: %s\n", msg);
	}

	return 0;
}


int send_flags(flag_t flags)
{
	int size;
	char flagmsg[32];

	size = sprintf(flagmsg, CMD_FLAGS " %u:%hu", id, flags);

	if (1 > send(udp_sock, flagmsg, size, 0)) {
		return -1;
	}

	return 0;
}
