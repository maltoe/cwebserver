/*
 * main.c
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#include "networking.h"
#include "resources.h"

#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

void print_usage()
{
	printf("Usage:\n");
	printf("\tcwebserver wwwpath [port]");
}

void on_sigint(int sig)
{
	printf("SIGINT received...\n");
	// Exit main loop
	net_exit();
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		print_usage();
		return 1;
	}

	// Initialize resources module
	if(res_set_www_path(argv[1]) == RES_INVALID_PATH)
	{
		print_usage();
		return 1;
	}

	// Read port number if given
	int port = 80;
	if(argc == 3)
	{
		port = atoi(argv[2]);
		if((port < 80) || (port > 65535))
			port = 80;
	}

	// Initialize networking module
	if(net_start_up(port) != NET_OK)
	{
		res_clean_up();
		return 1;
	}

	// Attach signal handler to SIGINT
	signal(SIGINT, on_sigint);

	// Enter main loop
	net_main_loop();

	res_clean_up();
	return 0;
}
