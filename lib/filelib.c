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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "compat.h"
#include "filelib.h"
#include "util.h"
#include "debug.h"
#include <assert.h>

#define TEMP_STR_LEN	(MAX_FILENAME*2)

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	filelib_init(BOOL do_count, BOOL keep_incomplete);
error_code	filelib_start(char *filename);
error_code	filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath);
error_code	filelib_write(char *buf, u_long size);
error_code	filelib_set_output_directory(char *str);
void		filelib_shutdown();
error_code  filelib_remove(char *filename);


/*********************************************************************************
 * Private Functions
 *********************************************************************************/
static error_code	mkdir_if_needed(char *str);
static void			close_file();
static BOOL			file_exists(char *filename);
static void trim_filename(char *filename, char* out);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static FHANDLE 	m_file;
static int 	m_count;
static char 	m_output_directory[MAX_PATH];
static char 	m_incomplete_directory[MAX_PATH];
static char 	m_filename_format[] = "%s%s.mp3";
static BOOL	m_keep_incomplete = TRUE;


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

error_code filelib_init(BOOL do_count, BOOL keep_incomplete)
{
	m_file = INVALID_FHANDLE;
	m_count = do_count ? 1 : -1;
	m_keep_incomplete = keep_incomplete;
	memset(&m_output_directory, 0, MAX_PATH);

	return SR_SUCCESS;
}


error_code filelib_set_output_directory(char *str)
{
	if (!str)
	{
		//JCBUG does this make sense?
		memset(&m_output_directory, 0, MAX_DIR_LEN);	
		return SR_SUCCESS;
	}
	strncpy(m_output_directory, str, MAX_BASE_DIR_LEN);
	mkdir_if_needed(m_output_directory);
	add_trailing_slash(m_output_directory);

	// Make the incomplete directory
	sprintf(m_incomplete_directory, "%s%s", m_output_directory, "incomplete");
	mkdir_if_needed(m_incomplete_directory);
	add_trailing_slash(m_incomplete_directory);

	{
		long t = strlen(m_incomplete_directory);
	}
	return SR_SUCCESS;
}

void close_file()
{
	if (m_file)
	{
		CloseFile(m_file);
		m_file = INVALID_FHANDLE;
	}
}

BOOL file_exists(char *filename)
{
	FHANDLE f = OpenFile(filename);

 	if (f == INVALID_FHANDLE)
	{
		return FALSE;
	}

	CloseFile(f);
	return TRUE;
}

error_code filelib_start(char *filename)
{
	char newfile[TEMP_STR_LEN];
	char tfile[TEMP_STR_LEN];
	long temp = 0;
	close_file();
	
	trim_filename(filename, tfile);
	sprintf(newfile, m_filename_format, m_incomplete_directory, tfile);
	temp = strlen(newfile);
	temp = strlen(tfile);
	temp = MAX_PATH_LEN;
	temp = strlen(m_incomplete_directory);

	if (m_keep_incomplete)
	{
		int n = 1;
		char oldfilename[TEMP_STR_LEN];
		char oldfile[TEMP_STR_LEN];
		strcpy(oldfilename, tfile);
		sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile);
		while(file_exists(oldfile))
		{
			sprintf(oldfilename, "%s(%d)", tfile, n);
			sprintf(oldfile, m_filename_format, m_incomplete_directory, oldfilename);
			n++;
		}
		if (strcmp(newfile, oldfile) != 0)
			MoveFile(newfile, oldfile);
	}

#if WIN32	
	m_file = CreateFile(newfile, GENERIC_WRITE,              // open for reading 
					FILE_SHARE_READ,           // share for reading 
					NULL,                      // no security 
					CREATE_ALWAYS,             // existing file only 
					FILE_ATTRIBUTE_NORMAL,     // normal file 
					NULL);                     // no attr. template 
 	if (m_file == INVALID_HANDLE_VALUE)
	{
		int r = GetLastError();
		r = strlen(newfile);
		return SR_ERROR_CANT_CREATE_FILE;
	}
#else
	// Needs to be better tested
	m_file = OpenFile(newfile);
	if (m_file == INVALID_FHANDLE)
	{
		return SR_ERROR_CANT_CREATE_FILE;
	}
#endif
	return SR_SUCCESS;
}

// Moves the file from incomplete to output directory
// Needs to be tested, added ok_to_write = FALSE, before it wasn't even
// initilzed. 
// *** fullpath must be over MAX_FILENAME len
error_code filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath)
{
	BOOL ok_to_write = TRUE;
	FHANDLE test_file;
	char newfile[TEMP_STR_LEN];
	char oldfile[TEMP_STR_LEN];
	char tfile[TEMP_STR_LEN];
	
	trim_filename(filename, tfile);

	close_file();

	// Make new paths for the old path and new
	memset(newfile, 0, TEMP_STR_LEN);
	memset(oldfile, 0, TEMP_STR_LEN);
	sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile);
	
	if (m_count != -1)
		sprintf(newfile, "%s%03d_%s.mp3", m_output_directory, m_count, tfile);
	else
		sprintf(newfile, m_filename_format, m_output_directory, tfile);

	// If we are over writing exsiting tracks
	if (!over_write_existing)
	{
		ok_to_write = FALSE;

		// test if we can open the file we would be overwriting
#if WIN32
		OutputDebugString(newfile);
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
			CloseFile(test_file);
	}

	if (ok_to_write)
	{
		// JCBUG -- clean this up
		int x = DeleteFile(newfile);
		x = MoveFile(oldfile, newfile);
//		if (!MOVE_FILE(oldfile, newfile))
//		{
//			int x = GetLastError();
//			return SR_ERROR_FAILED_TO_MOVE_FILE;
//		}
	}
	
	if (fullpath)
		strcpy(fullpath, newfile); 

	if (m_count != -1)
		m_count++;
	return SR_SUCCESS;
}


error_code filelib_write(char *buf, u_long size)
{
	if (!m_file)
	{
		DEBUG1(("trying to write to a non file"));
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
	filelib_init(FALSE, TRUE);
}

error_code filelib_remove(char *filename)
{
	char delfile[MAX_FILENAME];
	
	sprintf(delfile, m_filename_format, m_output_directory, filename);
	if (!DeleteFile(delfile))
		return SR_ERROR_FAILED_TO_MOVE_FILE;

	return SR_SUCCESS;
}

void trim_filename(char *filename, char* out)
{
	long maxlen = MAX_PATH_LEN-strlen(m_incomplete_directory);
	
	strncpy(out, filename, MAX_TRACK_LEN);
	strip_invalid_chars(out);
	out[maxlen-4] = '\0';	// -4 for ".mp3"
}
