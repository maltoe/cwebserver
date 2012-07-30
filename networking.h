/*
 * networking.h
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#ifndef NETWORKING_H_
#define NETWORKING_H_

/**************************** Module types & constants ***********************/

extern const int NET_OK;
extern const int NET_SOCKET_ERROR;
extern const int NET_BIND_ERROR;
extern const int NET_LISTEN_ERROR;

/**************************** Module interface *******************************/

/*
 * This should be called to start the network.
 */
int net_start_up(int port);

/*
 * This is a main loop which does the following:
 * - Select on the sockets
 * - Accept incoming connections OR
 * - Read data from the connected clients
 */
void net_main_loop();

/*
 * This method should be called to stop the main loop.
 */
void net_exit();

#endif /* NETWORKING_H_ */
