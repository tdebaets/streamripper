/* filelib.c - jonclegg@yahoo.com
 * library routines for file operations
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "filelib.h"
#include "util.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	filelib_init(BOOL do_count);
error_code	filelib_start(char *filename);
error_code	filelib_end(char *filename, BOOL over_write_existing);
error_code	filelib_write(char *buf, u_long size);
error_code	filelib_set_output_directory(char *str);
void		filelib_shutdown();
error_code  filelib_remove(char *filename);


/*********************************************************************************
 * Private Functions
 *********************************************************************************/
static error_code	mkdir_if_needed(char *str);
static void close_file();


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
#if WIN32
#define MOVE_FILE(oldfile, newfile)	MoveFile(oldfile, newfile)
#define CLOSE_FILE(file)			CloseHandle(file)
#define REMOVE_FILE(file)           DeleteFile(file)
#define HFILE						HANDLE
#else
#define MOVE_FILE(oldfile, newfile)	(!rename(oldfile, newfile))
#define CLOSE_FILE(file)			close(file)
#define REMOVE_FILE(file)           (!unlink(file))
#define HFILE						int
#define DWORD						unsigned long
#endif


static HFILE m_file;
static int m_count;
static char m_output_directory[MAX_PATH];
static char m_incomplete_directory[MAX_PATH];
static char m_filename_format[] = "%s%s.mp3";




// For now we're not going to care. If it makes it good. it not, will know 
// When we try to create a file in the path.
error_code mkdir_if_needed(char *str)
{
#if WIN32
	mkdir(str);
#else
	mkdir(str, 0777);
#endif
	return SR_SUCCESS;

}

error_code filelib_init(BOOL do_count)
{
	m_file = (HFILE)NULL;
	m_count = do_count ? 1 : -1;
	memset(&m_output_directory, 0, MAX_PATH);

	return SR_SUCCESS;
}


error_code filelib_set_output_directory(char *str)
{
	if (!str)
	{
		memset(&m_output_directory, 0, MAX_PATH);
		return SR_SUCCESS;
	}
	mkdir_if_needed(str);
//debug_printf(str);
	strcpy(m_output_directory, str);
	add_trailing_slash(m_output_directory);
//debug_printf(m_output_directory);

	// Make the incomplete directory
	sprintf(m_incomplete_directory, "%s%s", m_output_directory, "incomplete");
	mkdir_if_needed(m_incomplete_directory);
	add_trailing_slash(m_incomplete_directory);

	return SR_SUCCESS;
}

void close_file()
{
	if (m_file)
	{
		CLOSE_FILE(m_file);
		m_file = (HFILE)NULL;
	}
}

error_code filelib_start(char *filename)
{
	char newfile[MAX_FILENAME];
	close_file();

	sprintf(newfile, m_filename_format, m_incomplete_directory, filename);
	
#if WIN32	
	m_file = CreateFile(newfile, GENERIC_WRITE,              // open for reading 
					FILE_SHARE_READ,           // share for reading 
					NULL,                      // no security 
					CREATE_ALWAYS,             // existing file only 
					FILE_ATTRIBUTE_NORMAL,     // normal file 
					NULL);                     // no attr. template 
 	if (m_file == INVALID_HANDLE_VALUE)
		return SR_ERROR_CANT_CREATE_FILE;
#else
	// Needs to be better tested
	m_file = open(newfile, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
	if (m_file == -1)
	{
		return SR_ERROR_CANT_CREATE_FILE;
	}
#endif
	return SR_SUCCESS;
}

// Moves the file from incomplete to output directory
// Needs to be tested, added ok_to_write = FALSE, before it wasn't even
// initilzed. 
error_code filelib_end(char *filename, BOOL over_write_existing)
{
	BOOL ok_to_write = TRUE;
	HFILE test_file;
	char newfile[MAX_FILENAME];
	char oldfile[MAX_FILENAME];

	close_file();


	// Make new paths for the old path and new
	memset(newfile, 0, MAX_FILENAME);
	memset(oldfile, 0, MAX_FILENAME);
	sprintf(oldfile, m_filename_format, m_incomplete_directory, filename);
	
	if (m_count != -1)
		sprintf(newfile, "%s%03d_%s.mp3", m_output_directory, m_count, filename);
	else
		sprintf(newfile, m_filename_format, m_output_directory, filename);

	// If we are over writing exsiting tracks
	if (!over_write_existing)
	{
		ok_to_write = FALSE;

		// test if we can open the file we would be overwriting
#if WIN32
		test_file = CreateFile(newfile, GENERIC_WRITE, FILE_SHARE_READ, 
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
						NULL);
		if (test_file == INVALID_HANDLE_VALUE)
#else
		test_file = open(newfile, O_WRONLY | O_EXCL);
		if (test_file == -1)
#endif
			ok_to_write = TRUE;
		else
			CLOSE_FILE(test_file);
	}

	if (ok_to_write)
		if (!MOVE_FILE(oldfile, newfile))
			return SR_ERROR_FAILED_TO_MOVE_FILE;

	if (m_count != -1)
		m_count++;
	return SR_SUCCESS;
}


error_code filelib_write(char *buf, u_long size)
{
	if (!m_file)
	{
		debug_printf("trying to write to a non file\n");
		return SR_ERROR_CANT_WRITE_TO_FILE;
	}
#if WIN32
	{
	DWORD bytes_written = 0;
	if (!WriteFile(m_file, buf, size, &bytes_written, NULL))
		return SR_ERROR_CANT_WRITE_TO_FILE;
	}
#else
	if (write(m_file, buf, size) == -1)
		return SR_ERROR_CANT_WRITE_TO_FILE;
#endif
		

	return SR_SUCCESS;
}

void filelib_shutdown()
{
	close_file();

	/* 
    	 * We're just calling this to zero out 
	 * the vars, it's not really nessasary.
	 */
	filelib_init(FALSE);
}

error_code filelib_remove(char *filename)
{
	char delfile[MAX_FILENAME];
	
	sprintf(delfile, m_filename_format, m_output_directory, filename);
	if (!REMOVE_FILE(delfile))
		return SR_ERROR_FAILED_TO_MOVE_FILE;

	return SR_SUCCESS;
}
