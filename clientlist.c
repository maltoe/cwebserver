/*
 * clientlist.c
 *
 * This is a simple linked list to store the socket file descriptors in.
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#include "clientlist.h"

#include <stdlib.h>

/**************************** Global constants *******************************/

const int CLS_OK = 0;
const int CLS_NO_SUCH_ELEMENT = -1;

/**************************** Local types ************************************/

typedef struct __cls_entry
{
	int socketFd;
	struct __cls_entry* next;
} _cls_entry;

/**************************** Local variables ********************************/

_cls_entry* _cls_anchor = NULL;

/**************************** Module interface *******************************/

void cls_add(int socketFd)
{
	// Create a new node
	_cls_entry* node = malloc(sizeof(_cls_entry));
	node->socketFd = socketFd;
	node->next = NULL;

	if(_cls_anchor == NULL)
	{
		_cls_anchor = node;
	}
	else
	{
		// Iterate to last element
		_cls_entry* cur = _cls_anchor;
		while(cur->next != NULL)
			cur = cur->next;
		// Append to list
		cur->next = node;
	}
}

int cls_remove(int socketFd)
{
	if(_cls_anchor == NULL)
		return CLS_NO_SUCH_ELEMENT;

	// remove anchor?
	if(_cls_anchor->socketFd == socketFd)
	{
		_cls_entry* old = _cls_anchor;
		_cls_anchor = old->next;
		free(old);
		return CLS_OK;
	}
	else
	{
		if(_cls_anchor->next == NULL)
			return CLS_NO_SUCH_ELEMENT;

		// Iterate to desired element
		_cls_entry* prev = _cls_anchor;
		_cls_entry* cur = _cls_anchor->next;
		while(cur->socketFd != socketFd)
		{
			if(cur->next == NULL)
			{
				return CLS_NO_SUCH_ELEMENT;
			}
			else
			{
				prev = cur;
				cur = cur->next;
			}
		}

		// Remove element
		prev->next = cur->next;
		free(cur);
		return CLS_OK;
	}
}

int cls_get_length()
{
	if(_cls_anchor == NULL)
	{
		return 0;
	}
	else
	{
		int count = 1;
		_cls_entry* cur = _cls_anchor;
		while(cur->next != NULL)
		{
			cur = cur->next;
			++count;
		}
		return count;
	}
}

int cls_get(int index)
{
	if(_cls_anchor == NULL)
		return CLS_NO_SUCH_ELEMENT;

	_cls_entry* cur = _cls_anchor;
	while(index > 0)
	{
		if(cur->next == NULL)
		{
			return CLS_NO_SUCH_ELEMENT;
		}
		else
		{
			cur = cur->next;
			--index;
		}
	}
	return cur->socketFd;
}

int cls_get_max()
{
	if(_cls_anchor == NULL)
		return CLS_NO_SUCH_ELEMENT;

	int max = -1;

	_cls_entry* cur = _cls_anchor;
	while(cur != NULL)
	{
		if(cur->socketFd > max)
			max = cur->socketFd;
		cur = cur->next;
	}

	return max;
}
