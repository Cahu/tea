#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Server.hh"

#define PORT 9999

using TEA::Server;

int main(void)
{
	Server serv(PORT);

	while (1) {
		serv.process_events(1);
	}

	exit(EXIT_SUCCESS);
}
