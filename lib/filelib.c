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
error_code filelib_start(char *filename);
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
error_code filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath);
error_code filelib_set_output_directory(char *str);
void filelib_shutdown();
error_code filelib_remove(char *filename);


/*********************************************************************************
 * Private Functions
 *********************************************************************************/
static error_code mkdir_if_needed (char *str);
static void close_file (FHANDLE* fp);
static void close_files ();
static error_code filelib_write (FHANDLE fp, char *buf, u_long size);
static BOOL file_exists (char *filename);
static void trim_filename (char *filename, char* out);
static void trim_mp3_suffix (char *filename, char* out);
static error_code filelib_open_for_write (FHANDLE* fp, char *filename);
static void set_show_filenames (void);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static FHANDLE 	m_file;
static FHANDLE 	m_show_file;
static FHANDLE  m_cue_file;
static int 	m_count;
static int      m_do_show;
static char 	m_output_directory[MAX_PATH];
static char 	m_incomplete_directory[MAX_PATH];
static char 	m_filename_format[] = "%s%s.mp3";
static BOOL	m_keep_incomplete = TRUE;
static int      m_max_filename_length;
static char 	m_show_name[MAX_PATH];
static char 	m_cue_name[MAX_PATH];


// For now we're not going to care. If it makes it good. it not, will know 
// When we try to create a file in the path.
error_code
mkdir_if_needed(char *str)
{
#if WIN32
    mkdir(str);
#else
    mkdir(str, 0777);
#endif
    return SR_SUCCESS;
}

error_code
filelib_init(BOOL do_count, BOOL keep_incomplete, BOOL do_show_file,
	     char* show_file_name)
{
    m_file = INVALID_FHANDLE;
    m_show_file = INVALID_FHANDLE;
    m_cue_file = INVALID_FHANDLE;
    m_count = do_count ? 1 : -1;
    m_keep_incomplete = keep_incomplete;
    memset(&m_output_directory, 0, MAX_PATH);
    m_show_name[0] = 0;
    m_do_show = do_show_file;

    if (do_show_file) {
	if (show_file_name && *show_file_name) {
	    trim_mp3_suffix (show_file_name, m_show_name);
	    if (strlen(m_show_name) > MAX_PATH - 5) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	} else {
	    char datebuf[50];												\
	    time_t now = time(NULL);										\
	    strftime (datebuf, 50, "%Y_%m_%d_%H_%M_%S", localtime(&now));
	    sprintf (m_show_name,"sr_program_%s",datebuf);
	}
#if defined (commenout)
	strcpy (m_cue_name, m_show_name);
	strcat (m_cue_name, ".cue");
	strcat (m_show_name, ".mp3");
#endif
	/* Normally I would open the show & cue files here, but
	    I don't yet know the output directory.  So do nothing until
	    first write (until above problem is fixed).  */
    }

    return SR_SUCCESS;
}

error_code
filelib_set_output_directory(char *str)
{
    if (!str) {
	//JCBUG does this make sense?
	memset(&m_output_directory, 0, MAX_DIR_LEN);	
	return SR_SUCCESS;
    }
    strncpy(m_output_directory, str, MAX_BASE_DIR_LEN);
    mkdir_if_needed(m_output_directory);
    add_trailing_slash(m_output_directory);

    // Make the incomplete directory
    sprintf(m_incomplete_directory, "%s%s", m_output_directory,
	    "incomplete");
    mkdir_if_needed(m_incomplete_directory);
    add_trailing_slash(m_incomplete_directory);
    return SR_SUCCESS;
}

void
filelib_set_max_filename_length (int mfl)
{
    m_max_filename_length = mfl - strlen("incomplete") - 1;
}

void close_file(FHANDLE* fp)
{
    if (*fp != INVALID_FHANDLE) {
	CloseFile(*fp);
	*fp = INVALID_FHANDLE;
    }
}

void close_files()
{
    close_file (&m_file);
    close_file (&m_show_file);
    close_file (&m_cue_file);
}

BOOL
file_exists(char *filename)
{
#if defined (commentout)
    FHANDLE f = OpenFile(filename);
#endif
    FHANDLE f;

#if defined (WIN32)
    f = CreateFile (filename, GENERIC_READ,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL, NULL);
#else
    f = open(filename, O_RDONLY);
#endif

    if (f == INVALID_FHANDLE) {
	return FALSE;
    }

    CloseFile(f);
    return TRUE;
}

error_code
filelib_write_cue(char *artist, char* title, int secs)
{
    static int track_no = 1;
    int rc;
    char buf[1024];

    if (!m_do_show) return SR_SUCCESS;
    if (!m_cue_file) return SR_SUCCESS;

#if defined (commentout)
    /* Oops, forgot that Jon doesn't like the easy way... */
    fprintf (m_cue_file, "  TRACK %02d AUDIO\n",track_no++);
    fprintf (m_cue_file, "    TITLE \"%s\"\n",title);
    fprintf (m_cue_file, "    PERFORMER \"%s\"\n",artist);
    fprintf (m_cue_file, "    INDEX 01 %02d:%02d\n",
	secs / 60, secs % 60);
#endif
    rc = snprintf(buf,1024,"  TRACK %02d AUDIO\n",track_no++);
    filelib_write(m_cue_file,buf,rc);
    rc = snprintf(buf,1024,"    TITLE \"%s\"\n",title);
    filelib_write(m_cue_file,buf,rc);
    rc = snprintf(buf,1024,"    PERFORMER \"%s\"\n",artist);
    filelib_write(m_cue_file,buf,rc);
    rc = snprintf(buf,1024,"    INDEX 01 %02d:%02d\n",
	secs / 60, secs % 60);
    filelib_write(m_cue_file,buf,rc);

    return SR_SUCCESS;
}

error_code
filelib_start(char *filename)
{
    char newfile[TEMP_STR_LEN];
    char tfile[TEMP_STR_LEN];
    long temp = 0;
    close_file(&m_file);
	
    trim_filename(filename, tfile);
    sprintf(newfile, m_filename_format, m_incomplete_directory, tfile);
    temp = strlen(newfile);
    temp = strlen(tfile);
    temp = MAX_PATH_LEN;
    temp = strlen(m_incomplete_directory);

    if (m_keep_incomplete) {
	int n = 1;
	char oldfilename[TEMP_STR_LEN];
	char oldfile[TEMP_STR_LEN];
	strcpy(oldfilename, tfile);
	sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile);
	while(file_exists(oldfile)) {
	    sprintf(oldfilename, "%s(%d)", tfile, n);
	    sprintf(oldfile, m_filename_format, m_incomplete_directory, oldfilename);
	    n++;
	}
	if (strcmp(newfile, oldfile) != 0)
	    MoveFile(newfile, oldfile);
    }

    return filelib_open_for_write(&m_file, newfile);
}

// Moves the file from incomplete to output directory
error_code
filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath)
{
    BOOL ok_to_write = TRUE;
    FHANDLE test_file;
    char newfile[TEMP_STR_LEN];
    char oldfile[TEMP_STR_LEN];
    char tfile[TEMP_STR_LEN];

    trim_filename(filename, tfile);

    close_file (&m_file);

    // Make new paths for the old path and new
    memset(newfile, 0, TEMP_STR_LEN);
    memset(oldfile, 0, TEMP_STR_LEN);
    sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile);

    if (m_count != -1)
        sprintf(newfile, "%s%03d_%s.mp3", m_output_directory, m_count, tfile);
    else
	sprintf(newfile, m_filename_format, m_output_directory, tfile);

    // If we are over writing existing tracks
    if (!over_write_existing) {
	ok_to_write = FALSE;

	// test if we can open the file we would be overwriting
#if WIN32
	OutputDebugString(newfile);
	test_file = CreateFile(newfile, GENERIC_WRITE, FILE_SHARE_READ, 
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
				NULL);
#else
	test_file = open(newfile, O_WRONLY | O_EXCL);
#endif
	if (test_file == INVALID_FHANDLE)
	    ok_to_write = TRUE;
	else
	    CloseFile(test_file);
    }

    if (ok_to_write) {
	// JCBUG -- clean this up
	int x = DeleteFile(newfile);
	x = MoveFile(oldfile, newfile);
    }

    if (fullpath)
	strcpy(fullpath, newfile); 
    if (m_count != -1)
	m_count++;
    return SR_SUCCESS;
}

static error_code
filelib_open_for_write(FHANDLE* fp, char* filename)
{
#if WIN32	
    *fp = CreateFile(filename, GENERIC_WRITE,    // open for reading 
			FILE_SHARE_READ,           // share for reading 
			NULL,                      // no security 
			CREATE_ALWAYS,             // existing file only 
			FILE_ATTRIBUTE_NORMAL,     // normal file 
			NULL);                     // no attr. template 
    if (*fp == INVALID_FHANDLE)
    {
	int r = GetLastError();
	r = strlen(filename);
	printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#else
    // Needs to be better tested
    *fp = OpenFile(filename);
    if (*fp == INVALID_FHANDLE)
    {
	return SR_ERROR_CANT_CREATE_FILE;
    }
#endif
    return SR_SUCCESS;
}

error_code
filelib_write(FHANDLE fp, char *buf, u_long size)
{
    if (!fp) {
	DEBUG1(("trying to write to a non file"));
	return SR_ERROR_CANT_WRITE_TO_FILE;
    }
#if WIN32
    {
	DWORD bytes_written = 0;
	if (!WriteFile(fp, buf, size, &bytes_written, NULL))
	    return SR_ERROR_CANT_WRITE_TO_FILE;
    }
#else
    if (write(fp, buf, size) == -1)
	return SR_ERROR_CANT_WRITE_TO_FILE;
#endif

    return SR_SUCCESS;
}

error_code
filelib_write_track(char *buf, u_long size)
{
    return filelib_write (m_file, buf, size);
}

error_code
filelib_write_show(char *buf, u_long size)
{
    if (!m_do_show) return SR_SUCCESS;
    if (m_show_file != INVALID_FHANDLE) {
        return filelib_write (m_show_file, buf, size);
    }
    if (*m_show_name) {
	int rc;
	set_show_filenames ();
	rc = filelib_open_for_write (&m_cue_file, m_cue_name);
	if (rc != SR_SUCCESS) {
	    *m_show_name = 0;
	    return rc;
	}
	rc = filelib_open_for_write (&m_show_file, m_show_name);
	if (rc != SR_SUCCESS) {
	    *m_show_name = 0;
	    return rc;
	}
        rc = filelib_write (m_show_file, buf, size);
	if (rc != SR_SUCCESS) {
	    *m_show_name = 0;
	}
	return rc;
    }
    return SR_SUCCESS;
}

void
filelib_shutdown()
{
    close_files();

    /* 
     * We're just calling this to zero out 
     * the vars, it's not really nessasary.
     */
    filelib_init(FALSE, TRUE, FALSE, 0);
}

error_code
filelib_remove(char *filename)
{
    char delfile[MAX_FILENAME];
	
    sprintf(delfile, m_filename_format, m_output_directory, filename);
    if (!DeleteFile(delfile))
	return SR_ERROR_FAILED_TO_MOVE_FILE;

    return SR_SUCCESS;
}

static void
trim_filename(char *filename, char* out)
{
    long maxlen = m_max_filename_length;
    strncpy(out, filename, MAX_TRACK_LEN);
    strip_invalid_chars(out);
    out[maxlen-4] = '\0';	// -4 for ".mp3"
}

static void
trim_mp3_suffix(char *filename, char* out)
{
    char* suffix_ptr;
    strncpy(out, filename, MAX_PATH);
    suffix_ptr = out + strlen(out) - 4;  // -4 for ".mp3"
    if (strcmp(suffix_ptr,".mp3") == 0) {
	*suffix_ptr = 0;
    }
}

static int
is_absolute_path (char* fn)
{
#if WIN32
    if (strchr(fn,':')) {
	return 1;
    }
    if (*fn == '\\') {
	return 1;
    }
#endif
    if (*fn == '/') {
	return 1;
    }
    return 0;
}

static void
set_show_filenames (void)
{
    if (!*m_show_name) return;
    if (is_absolute_path(m_show_name)) {
	strcpy (m_cue_name, m_show_name);
    } else {
        snprintf (m_cue_name, MAX_PATH, "%s/%s", m_output_directory, m_show_name);
	strcpy (m_show_name, m_cue_name);
    }
    strcat (m_cue_name, ".cue");
    strcat (m_show_name, ".mp3");
}
