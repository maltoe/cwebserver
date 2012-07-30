/*
 * resources.c
 *
 * This file contains the module 'resources'. It is responsible
 * for loading external files and reading their length and mime type.
 *
 *  Created on: 09.01.2010
 *  	Author: Johannes Greiner <johannes.greiner@inf.fu-berlin.de>
 *      Author: Malte Rohde <malte.rohde@inf.fu-berlin.de>
 */

#include "base.h"
#include "resources.h"

#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

/**************************** Prototypes *************************************/

int _res_file_accessable(const char* path);
BOOL _res_dir_accessable(const char* path);
BOOL _res_sufficient_rights(const mode_t mode, const uid_t uid, const gid_t gid);
BOOL _res_known_file_type(const char* file, struct res_resource* resinfo);
char *_res_get_real_path(const char* relPath);

/**************************** Global constants *******************************/

const int RES_OK = 0;
const int RES_FILE_NOT_FOUND = 1;
const int RES_UNKNOWN_FILE_TYPE = 2;
const int RES_INVALID_PATH = 3;
const int RES_ACCESS_DENIED = 4;
const int RES_IO_ERROR = 5;

/**************************** Local variables ********************************/

/* the global www path */
char* _res_www_path;

/* length of the www path */
int _res_www_path_len;

/**************************** Module interface *******************************/

int res_set_www_path(char* path)
{
	// make sure path exists and is readable
	if(_res_dir_accessable(path) == FALSE)
		return RES_INVALID_PATH;

	// Make sure the www path ends with a slash.
	int pathLen = strlen(path);
	if(path[pathLen-1] != '/')
	{
		_res_www_path = malloc(pathLen + 2);
		strcpy(_res_www_path, path);
		_res_www_path[pathLen] = '/';
		_res_www_path_len = pathLen + 1;
	}
	else
	{
		_res_www_path = malloc(pathLen + 1);
		strcpy(_res_www_path, path);
		_res_www_path_len = pathLen;
	}

	return RES_OK;
}

int res_lookup(const char* path, struct res_resource* resinfo)
{
	// Check for forbidden .. in path
	if(strstr(path, "..") != NULL)
		return RES_INVALID_PATH;

	// Build full path
	char* realPath = _res_get_real_path(path);

	// Check for existance and access rights
	int access = _res_file_accessable(realPath);
	if(access != RES_OK)
		return access;

	// Get mime type
	if(_res_known_file_type(realPath, resinfo) == FALSE)
	{
		free(realPath);
		return RES_UNKNOWN_FILE_TYPE;
	}

	// Open file
	resinfo->file = fopen(realPath, "r");
	if(resinfo->file == NULL)
	{

		free(realPath);
		return RES_IO_ERROR;
	}

	// Determine the file length
	fseek(resinfo->file, 0, SEEK_END);
	resinfo->len = ftell(resinfo->file);
	fseek(resinfo->file, 0, SEEK_SET);

	return RES_OK;
}

void res_clean_up(void)
{
	free(_res_www_path);
}

/**************************** Local methods **********************************/

/*
 * Checks for existance of a file and whether it is readable.
 * Checks that it actually is a file, too.
 * Returns RES_OK on success, RES_FILE_NOT_FOUND or RES_ACCESS_DENIED else.
 */
int _res_file_accessable(const char* path)
{
	struct stat s;
	if(stat(path, &s) != 0)
	{
		return RES_FILE_NOT_FOUND;
	}
	else
	{
		if((s.st_mode & S_IFREG) &&
			(_res_sufficient_rights(s.st_mode, s.st_uid, s.st_gid) == TRUE))
			return RES_OK;
		else
			return RES_ACCESS_DENIED;
	}
}

/*
 * Checks for existance of a directory and whether it is readable.
 */
BOOL _res_dir_accessable(const char* path)
{
	struct stat s;
	if(stat(path, &s) != 0)
		return FALSE;
	else
	{
		if(s.st_mode & S_IFDIR)
			return _res_sufficient_rights(s.st_mode, s.st_uid, s.st_gid);
		else
			return FALSE;
	}
}

/*
 * Returns TRUE if the Webserver has sufficient rights to read from a file
 * or directory.
 */
BOOL _res_sufficient_rights(const mode_t mode, const uid_t uid, const gid_t gid)
{
	if(mode & S_IROTH)
		return TRUE;

	if((mode & S_IRGRP) && (gid == getgid()))
		return TRUE;

	if((mode & S_IRUSR) && (uid == getuid()))
		return TRUE;

	return FALSE;
}

/*
 * Checks whether 'file' has a known file extension and translates that
 * into a mime type, which is written into resinfo->mime.
 * Returns TRUE, if it was a known file.
 */
BOOL _res_known_file_type(const char* file, struct res_resource* resinfo)
{
	int extPos = strlen(file) - 3;

	// Sanity check: File must not only be '.ext'
	if(extPos <= 0)
		return FALSE;

	const char* ext = &file[extPos];

	// Check for HTML file (only 'tml')
	if(strcmp(ext, "tml") == 0)
	{
		strcpy(resinfo->mime, "text/html");
		return TRUE;
	}
	else if(strcmp(ext, "jpg") == 0)
	{
		strcpy(resinfo->mime, "image/jpeg");
		return TRUE;
	}
	else if(strcmp(ext, "gif") == 0)
	{
		strcpy(resinfo->mime, "image/gif");
		return TRUE;
	}
	else if(strcmp(ext, "png") == 0)
	{
		strcpy(resinfo->mime, "image/png");
		return TRUE;
	}

	return FALSE;
}

/*
 * Builds the real path of a relative path. Has to be free'd after use.
 */
char *_res_get_real_path(const char* relPath)
{
	int relPathLen = strlen(relPath);
	char* ret = malloc(_res_www_path_len + relPathLen);

	strcpy(ret, _res_www_path);

	// Remove leading slash if necessary
	if(relPath[0] == '/')
	{
		strcat(ret, &relPath[1]);
	}
	else
	{
		strcat(ret, relPath);
	}

	return ret;
}

