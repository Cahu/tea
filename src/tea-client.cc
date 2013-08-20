#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
	#include <assert.h>
#endif

#include <SDL/SDL.h>

#include "cmds.hh"
#include "keys.hh"     // key map
#include "Map.hh"
#include "Player.hh"
#include "NetGame.hh"
#include "graphics.hh"

#define PORT 9999


using TEA::Map;
using TEA::Player;


NetGame *ng;
static std::vector<Player *> players;


static int handle_net_events();
static int handle_sdl_events(unsigned int *);

static void add_player(unsigned int, double = 0, double = 0);
static void remove_player(unsigned int);
static void sync_player_pos(unsigned int, double, double);
static void sync_player_flags(unsigned int, unsigned int);



int main(int argc, char *argv[])
{
	init_sdl();
	init_opengl();
	init_world("map.txt");

	int exitval = EXIT_SUCCESS;

	// init state variables
	unsigned int flags = 0;

	try {
		ng = new NetGame(
			(argc < 2) ? "127.0.0.1" : argv[1],
			PORT
		);
	} catch (NetGame::Error &err) {
		fprintf(stderr, "Error #%d while connecting to server", err);
		exit(EXIT_FAILURE);
	}

	unsigned int id = ng->get_id();
	printf("Got id %u\n", id);

	// main loop
	while (1) {

		if (-1 == handle_net_events()) {
			break;
		}

		// events on mouse/keyboard
		switch (handle_sdl_events(&flags)) {
			case 0:
				// no event
				break;
			case -1:
				// quit
				goto END;
			default:
				if (ng->playing()) {
					ng->send_flags(flags);
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
		if (ng->playing()) {
			Player *p = players[id];
			draw_scene(players, p->get_xpos(), p->get_ypos());
		} else {
			draw_scene(players, 0, 0);
		}

		// sleep
		SDL_Delay(10);
	}

	END:
	delete ng;
	exit(exitval);
}



void add_player(unsigned int pid, double x, double y)
{
	if (pid >= players.size()) {
		players.resize(pid+1, NULL);
	}

#ifndef ndebug
	assert(players[pid] == NULL);
#endif

	players[pid] = new Player(x, y);
}


void remove_player(unsigned int pid)
{
	if (pid >= players.size() || players[pid] == NULL) {
		fprintf(stderr, "trying to remove a player we don't know about!\n");
		return;
	}

	delete players[pid];
	players[pid] = NULL;
}


void sync_player_pos(unsigned int pid, double x, double y)
{
	if (pid >= players.size() || players[pid] == NULL) {
		fprintf(
			stderr,
			"trying to sync an unknown player's pos (id = %u)!\n",
			pid
		);
		return;
	}

	players[pid]->set_pos(x, y);
}


void sync_player_flags(unsigned int pid, unsigned int flags)
{
	if (pid >= players.size() || players[pid] == NULL) {
		fprintf(
			stderr,
			"trying to sync an unknown player's flags (id = %u)!\n",
			pid
		);
		return;
	}

	players[pid]->set_flags(flags);
}


int handle_net_events()
{
	NetGame::Event e;

	try {
		ng->poll_events();
	} catch (NetGame::Error &err) {
		fprintf(stderr, "Error #%d while reading from server", err);
		return -1;
	}

	while (ng->pop_event(e)) {
		switch (e.type) {
			case NetGame::Event::PLAYER_ADD:
				add_player(e.pid, e.pxpos, e.pypos);
				break;
			case NetGame::Event::PLAYER_LEAVE:
				remove_player(e.pid);
				break;
			case NetGame::Event::PLAYER_SYNC:
				sync_player_pos(e.pid, e.pxpos, e.pypos);
				break;
			case NetGame::Event::PLAYER_FLAGS:
				sync_player_flags(e.pid, e.pflags);
				break;
			case NetGame::Event::SERVER_QUIT:
				puts("Server stopped the game.");
				return -1;
			case NetGame::Event::SERVER_KICK:
				puts("Kicked! Bad boy.");
				return -1;
			case NetGame::Event::SERVER_NEWGAME:
				break;
			case NetGame::Event::UNKNOWN:
				break;
		}
	}

	return 0;
}


int handle_sdl_events(unsigned int *flags)
{
	SDL_Event event;
	int got_event = 0;

	while (SDL_PollEvent(&event)) {
		got_event = 1;

		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
				case SDLK_k:
					ng->quit();
					return -1;
				case SDLK_j:
					if (!ng->playing()) {
						ng->join();
					}
					break;
				case SDLK_l:
					if (ng->playing()) {
						ng->leave();
					}
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
			ng->quit();
			return -1;
		}
	}

	return got_event;
}
