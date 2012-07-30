/*
 * clientlist.h
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#ifndef CLIENTLIST_H_
#define CLIENTLIST_H_

/**************************** Module constants *******************************/

extern const int CLS_OK;
extern const int CLS_NO_SUCH_ELEMENT;

/**************************** Module interface *******************************/

void cls_add(int socketFd);

/*
 * Returns
 * - CLS_OK on success
 * - CLS_NO_SUCH_ELEMENT else
 */
int cls_remove(int socketFd);

int cls_get_length();

/*
 * Returns CLS_NO_SUCH_ELEMENT if index is out of range.
 */
int cls_get(int index);

/*
 * Calculates the maximum file descriptor in the list.
 * Returns CLS_NO_SUCH_ELEMENT if the list is empty.
 */
int cls_get_max();

#endif /* CLIENTLIST_H_ */
