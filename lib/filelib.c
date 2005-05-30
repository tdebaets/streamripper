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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "compat.h"
#include "filelib.h"
#include "util.h"
#include "debug.h"
#include <assert.h>

#define TEMP_STR_LEN	(SR_MAX_PATH*2)

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code filelib_start(char *filename);
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
error_code filelib_end(char *filename, BOOL over_write_existing, BOOL truncate_dup, char *fullpath, char* a_pszPrefix);
void filelib_shutdown();
error_code filelib_remove(char *filename);


/*********************************************************************************
 * Private Functions
 *********************************************************************************/
static error_code device_split (char* dirname, char* device, char* path);
static error_code mkdir_if_needed (char *str);
static void close_file (FHANDLE* fp);
static void close_files ();
static error_code filelib_write (FHANDLE fp, char *buf, u_long size);
static BOOL file_exists (char *filename);
static void trim_filename (char *filename, char* out);
static void trim_mp3_suffix (char *filename, char* out);
static error_code filelib_open_for_write (FHANDLE* fp, char *filename);
static void set_show_filenames (void);
static int is_absolute_path (char* fn);
static error_code set_output_directories_old (char* output_directory,
					      int get_separate_dirs,
					      int get_date_stamp,
					      char* icy_name
					      );
static error_code set_output_directories_new (char* output_pattern,
					      char* output_directory,
					      int get_separate_dirs,
					      int get_date_stamp,
					      char* icy_name
					      );
static error_code sr_getcwd (char* dirbuf);
static error_code add_trailing_slash (char *str);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static FHANDLE 	m_file;
static FHANDLE 	m_show_file;
static FHANDLE  m_cue_file;
static int 	m_count;
static int      m_do_show;
static char 	m_output_directory[SR_MAX_PATH];
static char 	m_incomplete_directory[SR_MAX_PATH];
static char 	m_filename_format[] = "%s%s%s";
static BOOL	m_keep_incomplete = TRUE;
static int      m_max_filename_length;
static char 	m_show_name[SR_MAX_PATH];
static char 	m_cue_name[SR_MAX_PATH];
static char*	m_extension;
static BOOL	m_do_individual_tracks;

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
filelib_init (BOOL do_individual_tracks,
	      BOOL do_count,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* show_file_name,
	      char* output_directory,
	      char* output_pattern,
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name)
{
    error_code ret;

    m_file = INVALID_FHANDLE;
    m_show_file = INVALID_FHANDLE;
    m_cue_file = INVALID_FHANDLE;
    m_count = do_count ? 1 : -1;
    m_keep_incomplete = keep_incomplete;
    memset(&m_output_directory, 0, SR_MAX_PATH);
    m_show_name[0] = 0;
    m_do_show = do_show_file;
    m_do_individual_tracks = do_individual_tracks;

    switch (content_type) {
    case CONTENT_TYPE_MP3:
	m_extension = ".mp3";
	break;
    case CONTENT_TYPE_NSV:
    case CONTENT_TYPE_ULTRAVOX:
	m_extension = ".nsv";
	break;
    case CONTENT_TYPE_OGG:
	m_extension = ".ogg";
	break;
    case CONTENT_TYPE_AAC:
	m_extension = ".aac";
	break;
    default:
	fprintf (stderr, "Error (wrong suggested content type: %d)\n", 
	    content_type);
	return SR_ERROR_PROGRAM_ERROR;
    }

    if (do_show_file) {
	if (show_file_name && *show_file_name) {
	    trim_mp3_suffix (show_file_name, m_show_name);
	    if (strlen(m_show_name) > SR_MAX_PATH - 5) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	} else {
	    char datebuf[50];
	    time_t now = time(NULL);
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

    /* Get the path to the "parent" directory.  This is the directory
       that contains the incomplete dir and the show files.
       It might not contain the individual files if an output_pattern
       was specified. */
    if (output_pattern && *output_pattern) {
	set_output_directories_new (output_pattern,
				    output_directory,
				    get_separate_dirs,
				    get_date_stamp,
				    icy_name);
    } else {
	set_output_directories_old (output_directory,
				    get_separate_dirs,
				    get_date_stamp,
				    icy_name);
    }

    /* Finally, compute the amount of remaining path length for the 
     * music filenames */
    add_trailing_slash(m_incomplete_directory);
    m_max_filename_length = SR_MAX_PATH - strlen(m_incomplete_directory);

    return SR_SUCCESS;
}

/* This is the new way, using the -D flag and the stream name. */
error_code 
set_output_directories_new (char* output_pattern,
			    char* output_directory,
			    int get_separate_dirs,
			    int get_date_stamp,
			    char* icy_name
			    )
{
    error_code ret;
    char opat_device[3];
    char odir_device[3];
    char cwd_device[3];
    char* device;
    char opat_path[SR_MAX_PATH];
    char odir_path[SR_MAX_PATH];
    char cwd_path[SR_MAX_PATH];
    char cwd[SR_MAX_PATH];
    char pattern_buf[SR_MAX_PATH];
    char* default_pattern;

    char bp[SR_MAX_PATH];
    char* pp = output_pattern;
    int bi = 0;
    int pi = 0;

    /* Initialize strings */
    cwd[0] = '\0';
    odir_device[0] = '\0';
    opat_device[0] = '\0';
    odir_path[0] = '\0';
    opat_path[0] = '\0';
    ret = sr_getcwd (cwd);
    if (ret != SR_SUCCESS) return ret;
    if (get_separate_dirs) {
	default_pattern = "%S" PATH_SLASH_STR "%A - %T";
    } else {
	default_pattern = "%A - %T";
    }

    if (!output_pattern || !(*output_pattern)) {
	output_pattern = default_pattern;
    }

    /* Get the device. It can be empty. */
    if (output_directory && *output_directory) {
	device_split (output_directory, odir_device, odir_path);
    }
    device_split (output_pattern, opat_device, opat_path);
    device_split (cwd, cwd_device, cwd_path);
    if (*opat_device) {
	device = opat_device;
    } else if (*odir_device) {
	device = odir_device;
    } else {
	device = cwd_device;
    }

    /* Generate the output file pattern. */
    if (IS_ABSOLUTE_PATH(opat_path)) {
	cwd_path[0] = '\0';
	odir_path[0] = '\0';
    } else if (IS_ABSOLUTE_PATH(odir_path)) {
	cwd_path[0] = '\0';
    }
    if (*odir_path) {
	ret = add_trailing_slash(odir_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (*cwd_path) {
	ret = add_trailing_slash(cwd_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (strlen(device)+strlen(cwd_path)+strlen(opat_path)
	+strlen(odir_path) > SR_MAX_PATH-1) {
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
    sprintf (pattern_buf, "%s%s%s%s", device, cwd_path, 
	     odir_path, opat_path);

    /* <<LEFT OFF HERE>> */
    /* Parse & substitute the output pattern */
    while (bi < SR_MAX_PATH) {
	if (pp[pi] == '\0') {
	    break;
	}
	if (ISSLASH(pp[pi])) {
	    
	}
	if (pp[pi] != '%') {
	    bp[bi++] = pp[pi++];
	    continue;
	}
	switch (pp[pi+1]) {
	case '%':
	    bp[bi++]='%';
	    pi+=2;
	    continue;
	case '\0':
	default:
	    bp[bi++]='%';
	    pi++;
	    continue;
	}
    }

    return SR_SUCCESS;
}

/* This is the old way, using the -d & -s flags and the stream name. */
error_code 
set_output_directories_old (char* output_directory,
			    int get_separate_dirs,
			    int get_date_stamp,
			    char* icy_name
			    )
{
    error_code ret;
    char base_dir[SR_MAX_PATH];
    char *stripped_icy_name;
    unsigned int base_dir_len = 0;

    if (output_directory && *output_directory) {
#if defined (WIN32)
	if (_fullpath (base_dir, output_directory, SR_MAX_PATH) == NULL) {
	    return SR_ERROR_DIR_PATH_TOO_LONG;
	}
#else
	/* I wish I could do something like "realpath()" here, but it 
	 * doesn't have (e.g.) posix conformance. */
	if (is_absolute_path (output_directory)) {
	    strncpy (base_dir,output_directory,SR_MAX_PATH);
	    debug_printf("Had absolute path\n");
	} else {
	    char pwd[SR_MAX_PATH];
	    if (!getcwd (pwd, SR_MAX_PATH)) {
		debug_printf ("getcwd returned zero?\n");
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	    snprintf (base_dir, SR_MAX_PATH, "%s%c%s", pwd, 
		      PATH_SLASH,output_directory);
	    debug_printf("Had relative path\n");
	}
#endif
    } else {
	/* If no output dir specified, the base dir is pwd */
	debug_printf("No output directory specified.\n");
	ret = sr_getcwd (base_dir);
	if (!ret) return ret;
    }
    add_trailing_slash (base_dir);

    /* Next, get full path to station directory.  If !get_separate_dir,
     * then the station directory is just the base directory. */
    base_dir_len = strlen(base_dir);
    if (base_dir_len > SR_MAX_COMPLETE) {
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
    if (get_separate_dirs) {
	char timestring[SR_DATE_LEN+1];
	time_t timestamp;
	struct tm *theTime;
	int time_len = 0;
	int length_used = base_dir_len;
	int length_available;
	int rc;

	if (base_dir_len > SR_MAX_BASE) {
	    return SR_ERROR_DIR_PATH_TOO_LONG;
	}
	timestring[0] = '\0';
	if (get_date_stamp) {
	    if (base_dir_len > SR_MAX_BASE_W_DATE) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	    time(&timestamp);
	    theTime = localtime(&timestamp);
	    time_len = strftime(timestring, SR_DATE_LEN+1, "_%Y-%m-%d", theTime);
	    if (time_len != SR_DATE_LEN) {
		/* If my arithmetic is correct, this should never happen. */
		return SR_ERROR_PROGRAM_ERROR;
	    }
	    length_used += SR_DATE_LEN;
	}

	stripped_icy_name = strdup(icy_name);
	strip_invalid_chars(stripped_icy_name);
	left_str(stripped_icy_name, SR_MAX_PATH);
	trim(stripped_icy_name);

	/* Truncate the icy name if it's too long */
	length_available = SR_MAX_COMPLETE-length_used-strlen("/");
	if (strlen(stripped_icy_name) > length_available) {
	    stripped_icy_name[length_available] = 0;
	}

	rc = snprintf (m_output_directory, SR_MAX_COMPLETE, "%s%s%s%c",
		       base_dir,stripped_icy_name,timestring,PATH_SLASH);
	free(stripped_icy_name);
	if (rc < 0) {
	    /* If my arithmetic is correct, this should never happen. */
	    return SR_ERROR_PROGRAM_ERROR;
	}
    } else {
	strcpy (m_output_directory, base_dir);
    }

    sprintf(m_incomplete_directory, "%s%s", m_output_directory,
	    "incomplete");

    /* GCS FIX: I should recursively create the base_dir */
    if (m_do_individual_tracks || m_do_show) {
	debug_printf("Trying to make base_dir: %s\n", base_dir);
	mkdir_if_needed(base_dir);

	debug_printf ("Trying to make m_output_directory: %s\n",
		      m_output_directory);
	mkdir_if_needed(m_output_directory);

	/* Next, make the incomplete directory */
	if (m_do_individual_tracks) {
	    mkdir_if_needed(m_incomplete_directory);
	}
    }
    return SR_SUCCESS;
}

static error_code
add_trailing_slash (char *str)
{
#if defined (commentout)
#if WIN32
    if (str[strlen(str)-1] != '\\')
	strcat(str, "\\");
#else
    if (str[strlen(str)-1] != '/')
	strcat(str, "/");
#endif
#endif

    int len = strlen(str);
    if (len >= SR_MAX_PATH-1)
	return SR_ERROR_DIR_PATH_TOO_LONG;
    if (!ISSLASH(str[strlen(str)-1]))
	strcat (str, PATH_SLASH_STR);
    return SR_SUCCESS;
}

/* Split off the device */
static error_code 
device_split (char* dirname,
	      char* device,
	      char* path
	      )
{
    int di;

    if (HAS_DEVICE(dirname)) {
	device[0] = dirname[0];
	device[1] = dirname[1];
	device[2] = '\0';
	di = 2;
    } else {
	device[0] = '\0';
	di = 0;
    }
    strcpy (path, &dirname[di]);
    return SR_SUCCESS;
}

static error_code 
sr_getcwd (char* dirbuf)
{
#if defined (WIN32)
    if (!_getcwd (dirbuf, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#else
    if (!getcwd (dirbuf, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#endif
    return SR_SUCCESS;
}

/* GCS Testing... I don't think this function is used any more. */
#if defined (commentout)
char* 
filelib_get_output_directory ()
{
    return m_output_directory;
}
#endif

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
filelib_write_cue(TRACK_INFO* ti, int secs)
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
    rc = snprintf(buf,1024,"    TITLE \"%s\"\n",ti->title);
    filelib_write(m_cue_file,buf,rc);
    rc = snprintf(buf,1024,"    PERFORMER \"%s\"\n",ti->artist);
    filelib_write(m_cue_file,buf,rc);
    rc = snprintf(buf,1024,"    INDEX 01 %02d:%02d:00\n",
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

    if (!m_do_individual_tracks) return SR_SUCCESS;

    close_file(&m_file);

    trim_filename(filename, tfile);
    sprintf (newfile, m_filename_format, m_incomplete_directory,
	     tfile, m_extension);
    temp = strlen(newfile);
    temp = strlen(tfile);
    temp = SR_MAX_PATH;
    temp = strlen(m_incomplete_directory);

    if (m_keep_incomplete) {
	int n = 1;
	char oldfilename[TEMP_STR_LEN];
	char oldfile[TEMP_STR_LEN];
	strcpy(oldfilename, tfile);
	sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile, m_extension);
	while(file_exists(oldfile)) {
	    sprintf(oldfilename, "%s(%d)", tfile, n);
	    sprintf(oldfile, m_filename_format, m_incomplete_directory, oldfilename, m_extension);
	    n++;
	}
	if (strcmp(newfile, oldfile) != 0)
	    MoveFile(newfile, oldfile);
    }

    return filelib_open_for_write(&m_file, newfile);
}

// Moves the file from incomplete to complete directory
// fullpath is an output parameter
error_code
filelib_end (char *filename, BOOL over_write_existing, BOOL truncate_dup, 
	     char *fullpath, char* a_pszPrefix)
{
    BOOL ok_to_write = TRUE;
    BOOL file_exists = FALSE;
    FHANDLE test_file;
    char newfile[TEMP_STR_LEN];
    char oldfile[TEMP_STR_LEN];
    char tfile[TEMP_STR_LEN];

    if (!m_do_individual_tracks) return SR_SUCCESS;

    trim_filename (filename, tfile);
    close_file (&m_file);

    // Make new paths for the old path and new
    memset(newfile, 0, TEMP_STR_LEN);
    memset(oldfile, 0, TEMP_STR_LEN);
    sprintf(oldfile, m_filename_format, m_incomplete_directory, tfile, m_extension);

    if (m_count != -1)
        sprintf (newfile, "%s%s%03d_%s%s", m_output_directory, a_pszPrefix, 
		 m_count, tfile, m_extension);
    else
	sprintf (newfile, m_filename_format, m_output_directory, tfile, m_extension);

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
	file_exists = (test_file != INVALID_FHANDLE);
	if (!file_exists)
	    ok_to_write = TRUE;
	else
	    CloseFile(test_file);
    }

    if (ok_to_write) {
	// JCBUG -- clean this up
	int x = DeleteFile(newfile);
	x = MoveFile(oldfile, newfile);
    } else {
	if (truncate_dup && file_exists) {
	    TruncateFile(oldfile);
	}
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
    debug_printf ("filelib_open_for_write: %s\n", filename);
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
	printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#endif
    return SR_SUCCESS;
}

error_code
filelib_write(FHANDLE fp, char *buf, u_long size)
{
    if (!fp) {
	debug_printf("filelib_write: fp = 0\n");
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
        char cue_buf[1024];
	set_show_filenames ();
	rc = filelib_open_for_write (&m_cue_file, m_cue_name);
	if (rc != SR_SUCCESS) {
	    *m_show_name = 0;
	    return rc;
	}
	/* Write cue header here */
	rc = snprintf(cue_buf,1024,"FILE \"%s\" MP3\n",m_show_name);
	rc = filelib_write(m_cue_file,cue_buf,rc);
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
#if defined (commentout)
    filelib_init(TRUE, FALSE, TRUE, FALSE, CONTENT_TYPE_MP3, 0);
#endif
}

error_code
filelib_remove(char *filename)
{
    char delfile[SR_MAX_PATH];
	
    sprintf(delfile, m_filename_format, m_output_directory, filename, m_extension);
    if (!DeleteFile(delfile))
	return SR_ERROR_FAILED_TO_MOVE_FILE;

    return SR_SUCCESS;
}

/* GCS: This should get only the name, not the directory */
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
    strncpy(out, filename, SR_MAX_PATH);
    suffix_ptr = out + strlen(out) - 4;  // -4 for ".mp3"
    if (strcmp (suffix_ptr, m_extension) == 0) {
	*suffix_ptr = 0;
    }
}

static int
is_absolute_path (char* fn)
{
#if WIN32
    /* This function is not enough (e.g. c:foo.mpg), but should not
	be used in windows anyway. */
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
        snprintf (m_cue_name, SR_MAX_PATH, "%s/%s", 
		  m_output_directory, m_show_name);
	strcpy (m_show_name, m_cue_name);
    }
    strcat (m_cue_name, ".cue");
    strcat (m_show_name, m_extension);
}
