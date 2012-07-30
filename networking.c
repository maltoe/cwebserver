/*
 * networking.c
 *
 * This file contains all the networking stuff, like listening on a socket,
 * selecting, and reading from a socket.
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#include "base.h"
#include "networking.h"
#include "clientlist.h"
#include "resources.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/**************************** HTML error pages *******************************/

struct _net_html_error_page
{
	char* msg;
	char* content;
};

const struct _net_html_error_page _net_400_page =
{
		"400 Bad request",
		"<html><head><title>400 - Bad request</title></head><body><h3>Your HTTP request was malformed.</h3></body></html>"
};

const struct _net_html_error_page _net_401_page =
{
		"401 Access denied",
		"<html><head><title>401 - Access denied</title></head><body><h3>Access denied.</h3></body></html>"
};

const struct _net_html_error_page _net_404_page =
{
		"404 File not found",
		"<html><head><title>404 - File not found</title></head><body><h3>The page was not found.</h3></body></html>"
};

const struct _net_html_error_page _net_500_page =
{
		"500 Internal server error",
		"<html><head><title>500 - Internal server error</title></head><body><h3>An error happened while processing your request.</h3></body></html>"
};


/**************************** Prototypes *************************************/

BOOL _net_select(fd_set* fds);
void _net_accept_connections(fd_set* fds);
void _net_read_http_requests(fd_set* fds);
void _net_handle_http_request(char* request, int socket);
char* _net_get_resource_path(char* request);
void _net_send_resource(struct res_resource* resinfo, int socket);
void _net_send_error_page(const struct _net_html_error_page* error, int socket);
char* _net_generate_header(const char* status, int len, const char* mime);

/**************************** Global constants *******************************/

const int NET_OK = 0;
const int NET_SOCKET_ERROR = 1;
const int NET_BIND_ERROR = 2;
const int NET_LISTEN_ERROR = 3;

/**************************** Local variables ********************************/

/* BOOL indicating that the main loop should end */
BOOL _net_stop_main_loop;

/* the listening socket */
int _net_listening_socket;

/**************************** Module interface *******************************/

int net_start_up(int port)
{
	_net_stop_main_loop = FALSE;

	// Create the socket
	_net_listening_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(_net_listening_socket < 0)
	{
		fprintf(stderr, "Error: Could not create socket.\n");
		return NET_SOCKET_ERROR;
	}

	// Build a sockaddr
	struct sockaddr_in sAddr;
	memset(&sAddr, 0, sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = INADDR_ANY;
	sAddr.sin_port = htons(port);

	// Bind to port
	if(bind(_net_listening_socket, (struct sockaddr*) &sAddr, sizeof(struct sockaddr)) < 0)
	{
		fprintf(stderr, "Error: Could not bind to port 80. Are you root?\n");
		return NET_BIND_ERROR;
	}

	// Listen
	if(listen(_net_listening_socket, 5) < 0)
	{
		fprintf(stderr, "Error: Could not listen on port 80.\n");
		return NET_LISTEN_ERROR;
	}

	return NET_OK;
}

void net_main_loop()
{
	fd_set fds;

	while(_net_stop_main_loop == FALSE)
	{
		// Select
		if(_net_select(&fds) == FALSE)
		{
			_net_stop_main_loop = TRUE;
			break;
		}

		// Handle incoming connections
		_net_accept_connections(&fds);

		// Handle HTTP request
		_net_read_http_requests(&fds);
	}

	// Close the listening socket
	close(_net_listening_socket);
}

void net_exit()
{
	_net_stop_main_loop = TRUE;
}

/**************************** Local methods **********************************/

/*
 * Selects on the fd_set. Returns TRUE if select was successful, and FALSE
 * if select was interrupted (SIGINT) or failed.
 */
BOOL _net_select(fd_set* fds)
{
	// Clear the fd set
	FD_ZERO(fds);

	// Mark interesting fds
	FD_SET(_net_listening_socket, fds);
	int i;
	for(i = 0; i < cls_get_length(); ++i)
		FD_SET(cls_get(i), fds);

	// Determine highest fd number
	int maxFd = cls_get_max();
	if(maxFd == CLS_NO_SUCH_ELEMENT)
		maxFd = _net_listening_socket;

	// Select
	if(select(maxFd + 1, fds, NULL, NULL, NULL) == -1)
	{
        if(errno != EINTR)
        {
        	// Do not print this line in case of a signal as the signal handler takes care of it.
        	fprintf(stderr, "Error: Could not select on socket.\n");
        }
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/*
 * Accepts incoming connections and adds them to the clientlist.
 */
void _net_accept_connections(fd_set* fds)
{
	if(FD_ISSET(_net_listening_socket, fds))
	{
	    int connection_socket = accept(_net_listening_socket, NULL, NULL);

	    if(connection_socket < 0)
	    {
	    	fprintf(stderr, "Error: Could not accept connection.\n");
	    }
	    else
	    {
	    	// Push back the fd to the list
	    	cls_add(connection_socket);
	    }
	}
}

/*
 * Reads data from any (client) socket which has data to read.
 */
void _net_read_http_requests(fd_set* fds)
{
	int i;
	for(i = 0; i < cls_get_length(); ++i)
	{
		int curFd = cls_get(i);
		if(FD_ISSET(curFd, fds))
		{
			char buffer[200];
			memset(buffer, 0, 200);

			int bytesRead = read(curFd, buffer, 199);
			if(bytesRead > 0)
			{
				// Handle the http request and send a reply.
				_net_handle_http_request(buffer, curFd);
			}
			// else read error. Do nothing.

			// Close the socket.
			close(curFd);
			// Remove it from the client list.
			cls_remove(curFd);
		}
	}
}

/*
 * Reacts on a HTTP request (parsing, sending a specific answer)
 */
void _net_handle_http_request(char* request, int socket)
{
	char *resPath = _net_get_resource_path(request);
	if(resPath == NULL)
	{
		// The request could not be parsed.
		_net_send_error_page(&_net_400_page, socket);
		return;
	}

	struct res_resource resinfo;
	int lookupRet = res_lookup(resPath, &resinfo);

	// TODO: Somehow, RES_xxx do not work inside a switch statement.
	// gcc says:
	// "error: case label does not reduce to an integer constant"

	if(lookupRet == RES_OK)
	{
		_net_send_resource(&resinfo, socket);
		fclose(resinfo.file);
	}
	else if(lookupRet == RES_FILE_NOT_FOUND)
	{
		_net_send_error_page(&_net_404_page, socket);
	}
	else if((lookupRet == RES_INVALID_PATH) || (lookupRet == RES_ACCESS_DENIED))
	{
		// resPath contained '..'
		_net_send_error_page(&_net_401_page, socket);
	}
	else
	{
		/*
		 * RES_IO_ERROR
		 * RES_UNKNOWN_FILE_TYPE
		 */
		_net_send_error_page(&_net_500_page, socket);
	}
}

/*
 * Extracts the resource path from a HTTP request
 */
char* _net_get_resource_path(char* request)
{
	int reqLen = strlen(request);
	int pos = 0;

	// Skip the first word
	while(request[pos] != ' ')
	{
		if(++pos == reqLen)
			return NULL;
	}

	// Skip any following white space just in case...
	while(request[pos] == ' ')
	{
		if(++pos == reqLen)
			return NULL;
	}

	// Go to the next white space or line end
	int endPos = pos;
	while((request[endPos] != ' ') && (request[endPos] != '\n') && (request[endPos] != '\r'))
	{
		if(++endPos == reqLen)
			return NULL;
	}

	// Set an end mark right behind the resource path
	request[endPos] = '\0';

	return &request[pos];
}

/*
 * Sends a res_resource to the client.
 */
void _net_send_resource(struct res_resource* resinfo, int socket)
{
	// Generate header
	char *header = _net_generate_header("200 OK", resinfo->len, resinfo->mime);
	int headerLen = strlen(header);

	// Send header
	ssize_t bytesSent = send(socket, header, headerLen, 0);
	free(header);

	if(bytesSent != headerLen)
	{
		fprintf(stderr, "Error: Could not send to socket.");
		net_exit();
		return;
	}

	// Create buffer large enough to contain the whole file
	char *buf = malloc(sizeof(char) * resinfo->len);
	if(buf == NULL)
	{
		fprintf(stderr, "Error: Out of memory.\n");
		net_exit();
		return;
	}

	// Read file
	size_t bytesRead = fread(buf, 1, resinfo->len, resinfo->file);
	if(bytesRead != resinfo->len)
	{
		fprintf(stderr, "Error: Could not read from file.\n");
		net_exit();
	}
	else
	{
		// Send content
		bytesSent = send(socket, buf, resinfo->len, 0);
		if(bytesSent != resinfo->len)
		{
			net_exit();
		}
	}

	free(buf);
}

/*
 * Sends an error page to the client.
 */
void _net_send_error_page(const struct _net_html_error_page* error, int socket)
{
	int contentLen = strlen(error->content);

	// Generate header
	char *header = _net_generate_header(error->msg, contentLen, "text/html");
	int headerLen = strlen(header);

	// Send header
	ssize_t bytesSent = send(socket, header, headerLen, 0);
	free(header);

	if(bytesSent != headerLen)
	{
		fprintf(stderr, "Error: Could not send to socket.");
		net_exit();
		return;
	}

	// Send content
	bytesSent = send(socket, error->content, contentLen, 0);

	if(bytesSent != contentLen)
	{
		fprintf(stderr, "Error: Could not send to socket.");
		net_exit();
	}
}

/*
 * Generates a HTTP header including newline. Has to be free'd afterwards.
 */
char* _net_generate_header(const char* status, int len, const char* mime)
{
	char *header = malloc(sizeof(char) * 200);
	memset(header, 0, 200);

	strcpy(header, "HTTP/1.0 ");
	strcat(header, status);
	strcat(header, "\n");

	strcat(header, "Content-Length: ");
	char lenStr[10];
	sprintf(lenStr, "%i", len);
	strcat(header, lenStr);
	strcat(header, "\n");

	strcat(header, "Content-Type: ");
	strcat(header, mime);
	strcat(header, "\n");

	// Empty line
	strcat(header, "\n");

	return header;
}
