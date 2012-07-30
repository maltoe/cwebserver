/*
 * resources.h
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#ifndef RESOURCES_H_
#define RESOURCES_H_

#include <stdio.h> // for FILE

/**************************** Module types & constants ***********************/

/*
 * a resource contains all information about a file that
 * is needed for transmission
 */
struct res_resource
{
	FILE* file;	/* file pointer, already opened */
	char mime[15]; /* mime type */
	int len; /* file size in bytes */
};

/*
 * these constants have different meanings in the functions below.
 * See their description for explanation.
 */
extern const int RES_OK;
extern const int RES_FILE_NOT_FOUND;
extern const int RES_UNKNOWN_FILE_TYPE;
extern const int RES_INVALID_PATH;
extern const int RES_ACCESS_DENIED;
extern const int RES_IO_ERROR;

/**************************** Module interface *******************************/

/*
 * Sets the 'prefix' path for all resources (eg. '/var/www/').
 * This has to be called at startup.
 * Returns RES_OK on success or RES_INVALID_PATH if the path does not exist or is
 * not readable.
 */
int res_set_www_path(char* path);

/*
 * Lookup method. Used to find 'path' in the filesystem. If the file is found, the
 * resource struct is filled appropriately and RES_OK is returned. The FILE contained
 * in the resource struct should be closed after using.
 * Return values:
 * - RES_OK
 * - RES_FILE_NOT_FOUND : 'path' does not exist in the file system
 * - RES_INVALID_PATH : 'path' contained '..'
 * - RES_ACCESS_DENIED : 'path' is not a regular file or could not be accessed
 * - RES_UNKNOWN_FILE_TYPE : 'path' is neither a HTML file nor a GIF/JPEG image
 * - RES_IO_ERROR : 'path' could not be opened for reading.
 *
 * Neither path not resinfo may be NULL.
 */
int res_lookup(const char* path, struct res_resource* resinfo);

/*
 * Clean-up method. Has to be called when the server exits.
 */
void res_clean_up(void);


#endif /* RESOURCES_H_ */
