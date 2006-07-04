/* filelib.c
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
#include <ctype.h>
#include <errno.h>
#include "compat.h"
#include "filelib.h"
#include "util.h"
#include "debug.h"
#include <assert.h>
#include <sys/types.h>
#include "uce_dirent.h"

#define TEMP_STR_LEN	(SR_MAX_PATH*2)

/*****************************************************************************
 * Public functions
 *****************************************************************************/
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
void filelib_shutdown();
error_code filelib_remove(char *filename);


/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static error_code device_split (char* dirname, char* device, char* path);
static error_code mkdir_if_needed (char *str);
static error_code mkdir_recursive (char *str, int make_last);
static void close_file (FHANDLE* fp);
static void close_files ();
static error_code filelib_write (FHANDLE fp, char *buf, u_long size);
static BOOL file_exists (char *filename);
static void trim_filename (char *filename, char* out);
static void trim_mp3_suffix (char *filename);
static error_code filelib_open_for_write (FHANDLE* fp, char *filename);
static void
parse_and_subst_dir (char* pattern_head, char* pattern_tail, char* opat_path,
		     int is_for_showfile);
static void
parse_and_subst_pat (char* newfile,
		     TRACK_INFO* ti,
		     char* directory,
		     char* pattern,
		     char* extension);
static void set_default_pattern (BOOL get_separate_dirs, BOOL do_count);
static error_code 
set_output_directory (char* global_output_directory,
		      char* global_output_pattern,
		      char* output_pattern,
		      char* output_directory,
		      char* default_pattern,
		      char* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
		      );
static error_code sr_getcwd (char* dirbuf);
static error_code add_trailing_slash (char *str);
static int get_next_sequence_number (char* fn_base);
static void fill_date_buf (char* datebuf, int datebuf_len);
static error_code filelib_open_showfiles ();

/*****************************************************************************
 * Private Vars
 *****************************************************************************/
#define DATEBUF_LEN 50
static FHANDLE 	m_file;
static FHANDLE 	m_show_file;
static FHANDLE  m_cue_file;
static int 	m_count;
static int      m_do_show;
static char 	m_default_pattern[SR_MAX_PATH];
static char 	m_output_directory[SR_MAX_PATH];
static char 	m_output_pattern[SR_MAX_PATH];
static char 	m_incomplete_directory[SR_MAX_PATH];
static char     m_incomplete_filename[SR_MAX_PATH];
static char 	m_showfile_directory[SR_MAX_PATH];
static char     m_showfile_pattern[SR_MAX_PATH];
static BOOL	m_keep_incomplete = TRUE;
static int      m_max_filename_length;
static char 	m_show_name[SR_MAX_PATH];
static char 	m_cue_name[SR_MAX_PATH];
static char 	m_icy_name[SR_MAX_PATH];
static char*	m_extension;
static BOOL	m_do_individual_tracks;
static char     m_session_datebuf[DATEBUF_LEN];
static char     m_stripped_icy_name[SR_MAX_PATH];

// For now we're not going to care. If it makes it good. it not, will know 
// When we try to create a file in the path.
static error_code
mkdir_if_needed(char *str)
{
#if WIN32
    mkdir(str);
#else
    mkdir(str, 0777);
#endif
    return SR_SUCCESS;
}

/* Recursively make directories.  If make_last == 1, then the final 
   substring (after the last '/') is considered a directory rather 
   than a file name */
static error_code
mkdir_recursive (char *str, int make_last)
{
    char buf[SR_MAX_PATH];
    char* p = buf;
    char q;

    buf[0] = '\0';
    while ((q = *p++ = *str++) != '\0') {
	if (ISSLASH(q)) {
	    *p = '\0';
	    mkdir_if_needed (buf);
	}
    }
    if (make_last) {
	mkdir_if_needed (str);
    }
    return SR_SUCCESS;
}

error_code
filelib_init (BOOL do_individual_tracks,
	      BOOL do_count,
	      int count_start,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* output_directory,
	      char* output_pattern,
	      char* showfile_pattern,
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name)
{
    m_file = INVALID_FHANDLE;
    m_show_file = INVALID_FHANDLE;
    m_cue_file = INVALID_FHANDLE;
    m_count = do_count ? count_start : -1;
    m_keep_incomplete = keep_incomplete;
    memset(&m_output_directory, 0, SR_MAX_PATH);
    m_show_name[0] = 0;
    m_do_show = do_show_file;
    m_do_individual_tracks = do_individual_tracks;

    debug_printf ("FILELIB_INIT: output_directory=%s\n",
		  output_directory ? output_directory : "");
    debug_printf ("FILELIB_INIT: output_pattern=%s\n",
		  output_pattern ? output_pattern : "");
    debug_printf ("FILELIB_INIT: showfile_pattern=%s\n",
		  showfile_pattern ? showfile_pattern : "");

    sr_strncpy (m_icy_name, icy_name, SR_MAX_PATH);
    sr_strncpy (m_stripped_icy_name, icy_name, SR_MAX_PATH);
    debug_printf ("Stripping icy name...\n");
    strip_invalid_chars (m_stripped_icy_name);
    debug_printf ("Done stripping icy name.\n");

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

    /* Initialize session date */
    fill_date_buf (m_session_datebuf, DATEBUF_LEN);

    /* Set up the proper pattern if we're using -q and -s flags */
    set_default_pattern (get_separate_dirs, do_count);

    /* Get the path to the "parent" directory.  This is the directory
       that contains the incomplete dir and the show files.
       It might not contain the complete files if an output_pattern
       was specified. */
    set_output_directory (m_output_directory,
			  m_output_pattern,
			  output_pattern,
			  output_directory,
			  m_default_pattern,
			  "%A - %T",
			  get_separate_dirs,
			  get_date_stamp,
			  0);

    sprintf (m_incomplete_directory, "%s%s%c", m_output_directory,
	     "incomplete", PATH_SLASH);
    debug_printf ("Incomplete directory: %s\n", m_incomplete_directory);

    /* Recursively make the output directory & incomplete directory */
    if (m_do_individual_tracks) {
	debug_printf("Trying to make output_directory: %s\n", 
		     m_output_directory);
	mkdir_recursive (m_output_directory, 1);

	/* Next, make the incomplete directory */
	if (m_do_individual_tracks) {
	    mkdir_if_needed(m_incomplete_directory);
	}
    }

    /* Compute the amount of remaining path length for the filenames */
    m_max_filename_length = SR_MAX_PATH - strlen(m_incomplete_directory);

    /* Get directory and pattern of showfile */
    if (do_show_file) {
	if (showfile_pattern && *showfile_pattern) {
	    trim_mp3_suffix (showfile_pattern);
	    if (strlen(m_show_name) > SR_MAX_PATH - 5) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	}
	set_output_directory (m_showfile_directory,
			      m_showfile_pattern,
			      showfile_pattern,
			      output_directory,
			      "%S/sr_program_%d",
			      "",
			      get_separate_dirs,
			      get_date_stamp,
			      1);
	mkdir_recursive (m_showfile_directory, 1);
	filelib_open_showfiles ();
    }

    return SR_SUCCESS;
}

/* This sets the value for m_default_pattern, using the -q & -s flags */
static void
set_default_pattern (BOOL get_separate_dirs, BOOL do_count)
{
    /* None of these operations can overflow m_default_pattern */
    m_default_pattern[0] = '\0';
    if (get_separate_dirs) {
	strcpy (m_default_pattern, "%S" PATH_SLASH_STR);
    }
    if (do_count) {
	if (m_count < 0) {
	    strcat (m_default_pattern, "%q_");
	} else {
	    sprintf (&m_default_pattern[strlen(m_default_pattern)], 
		     "%%%dq_", m_count);
	}
    }
    strcat (m_default_pattern, "%A - %T");
}

/* This function sets the value of m_output_directory or 
   m_showfile_directory. */
static error_code 
set_output_directory (char* global_output_directory,
		      char* global_output_pattern,
		      char* output_pattern,
		      char* output_directory,
		      char* default_pattern,
		      char* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
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

    char pattern_head[SR_MAX_PATH];
    char pattern_tail[SR_MAX_PATH];


    debug_printf ("SET_OUTPUT_DIR:output_pattern=%s\n",output_pattern?output_pattern:"");
    debug_printf ("SET_OUTPUT_DIR:output_directory=%s\n",output_directory?output_directory:"");
    debug_printf ("SET_OUTPUT_DIR:default_pattern=%s\n",default_pattern?default_pattern:"");
    debug_printf ("SET_OUTPUT_DIR:default_pattern_tail=%s\n",default_pattern_tail?default_pattern_tail:"");

    /* Initialize strings */
    cwd[0] = '\0';
    odir_device[0] = '\0';
    opat_device[0] = '\0';
    odir_path[0] = '\0';
    opat_path[0] = '\0';
    ret = sr_getcwd (cwd);
    if (ret != SR_SUCCESS) return ret;

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
	/* No device */
	device = "";
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

    /* Fill in %S and %d patterns */
    sprintf (pattern_head, "%s%s%s", device, cwd_path, odir_path);
    debug_printf ("SET_OUTPUT_DIR:pattern_head(pre)=%s\n",pattern_head);
    debug_printf ("SET_OUTPUT_DIR:opat_path=%s\n",opat_path);
    parse_and_subst_dir (pattern_head, pattern_tail, opat_path, 
			 is_for_showfile);

    /* In case there is no %A, no %T, etc., use the default pattern */
    if (!*pattern_tail) {
	strcpy (pattern_tail, default_pattern_tail);
    }

    /* Set the global variables */
    strcpy (global_output_directory, pattern_head);
    add_trailing_slash (global_output_directory);
    strcpy (global_output_pattern, pattern_tail);

    return SR_SUCCESS;
}

/* Parse & substitute the output pattern.  What we're trying to
   get is everything up to the pattern specifiers that change 
   from track to track: %A, %T, %a, %D, %q, or %Q. 
   If %S or %d appear before this, substitute in. 
   If it's for the showfile, then we don't advance pattern_head 
   If there is no %A, no %T, etc.
*/
static void
parse_and_subst_dir (char* pattern_head, char* pattern_tail, char* opat_path,
		     int is_for_showfile)
{
    int opi = 0;
    unsigned int phi = 0;
    int ph_base_len;
    int op_tail_idx;

    phi = strlen(pattern_head);
    opi = 0;
    ph_base_len = phi;
    op_tail_idx = opi;

    while (phi < SR_MAX_BASE) {
	debug_printf ("::%d %d %d %d\n", phi, opi, ph_base_len, op_tail_idx);
	if (ISSLASH(opat_path[opi])) {
	    pattern_head[phi++] = PATH_SLASH;
	    opi++;
	    ph_base_len = phi;
	    op_tail_idx = opi;
	    continue;
	}
	if (opat_path[opi] == '\0') {
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	}
	if (opat_path[opi] != '%') {
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (opat_path[opi+1]) {
	case '%':
	    pattern_head[phi++]='%';
	    opi+=2;
	    continue;
	case 'S':
	    /* append stream name */
	    strncpy (&pattern_head[phi], m_stripped_icy_name, SR_MAX_BASE-phi);
	    phi = strlen (pattern_head);
	    opi+=2;
	    continue;
	case 'd':
	    /* append date info */
	    strncpy (&pattern_head[phi], m_session_datebuf, SR_MAX_BASE-phi);
	    phi = strlen (pattern_head);
	    opi+=2;
	    continue;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9': 
	case 'a':
	case 'A':
	case 'D':
	case 'q':
	case 'T':
	    /* These are track specific patterns */
	    break;
	case '\0':
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    pattern_head[phi++] = opat_path[opi++];
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	default:
	    /* This is an illegal pattern, so copy the '%' and continue */
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got to here, it means that we hit something like %A or %T */
	break;
    }
    /* Terminate the pattern_head string */
    debug_printf ("::%d %d %d %d\n", phi, opi, ph_base_len, op_tail_idx);
    pattern_head[ph_base_len] = 0;
    debug_printf ("Got pattern head: %s\n", pattern_head);
    debug_printf ("Got opat tail:    %s\n", &opat_path[op_tail_idx]);

    strcpy (pattern_tail, &opat_path[op_tail_idx]);
}

static void
fill_date_buf (char* datebuf, int datebuf_len)
{
    time_t now = time(NULL);
    strftime (datebuf, datebuf_len, "%Y_%m_%d_%H_%M_%S", localtime(&now));
}

static error_code
add_trailing_slash (char *str)
{
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

/* It's a bit touch and go here. My artist substitution might 
   be into a directory, in which case I don't have enough 
   room for a legit file name */
/* Also, what about versioning of completed filenames? */
/* If (TRACK_INFO* ti) is NULL, that means we're being called for the 
   showfile, and therefore some parts don't apply */
static void
parse_and_subst_pat (char* newfile,
		     TRACK_INFO* ti,
		     char* directory,
		     char* pattern,
		     char* extension)
{
    char stripped_artist[SR_MAX_PATH];
    char stripped_title[SR_MAX_PATH];
    char stripped_album[SR_MAX_PATH];
#define DATEBUF_LEN 50
    char temp[DATEBUF_LEN];
    char datebuf[DATEBUF_LEN];
    int opi = 0;
    int nfi = 0;
    int done;
    char* pat = pattern;

    /* Reserve 5 bytes: 4 for the .mp3 extension, and 1 for null char */
    int MAX_FILEBASELEN = SR_MAX_PATH-5;

    debug_printf ("OUTPUT PATTERN:%s\n", pattern);
    strcpy (newfile, directory);
    opi = 0;
    nfi = strlen(newfile);
    done = 0;

    /* Strip artist, title, album */
    if (ti) {
	sr_strncpy (stripped_artist, ti->artist, SR_MAX_PATH);
	sr_strncpy (stripped_title, ti->title, SR_MAX_PATH);
	sr_strncpy (stripped_album, ti->album, SR_MAX_PATH);
	strip_invalid_chars(stripped_artist);
	strip_invalid_chars(stripped_title);
	strip_invalid_chars(stripped_album);
    }

    while (nfi < MAX_FILEBASELEN) {
	debug_printf ("COMPOSING OUTPUT PATTERN:%s\n", newfile);
	if (pat[opi] == '\0') {
	    done = 1;
	    break;
	}
	if (pat[opi] != '%') {
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = '\0';
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (pat[opi+1]) {
	case '%':
	    newfile[nfi++]='%';
	    newfile[nfi] = '\0';
	    opi+=2;
	    continue;
	case 'S':
	    /* stream name */
	    /* oops */
	    strncat (newfile, m_icy_name, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'd':
	    /* append date info */
	    strncat (newfile, m_session_datebuf, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'D':
	    /* current timestamp */
	    fill_date_buf (datebuf, DATEBUF_LEN);
	    strncat (newfile, datebuf, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'a':
	    /* album */
	    if (!ti) goto illegal_pattern;
	    strncat (newfile, stripped_album, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'A':
	    /* artist */
	    if (!ti) goto illegal_pattern;
	    strncat (newfile, stripped_artist, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'q':
	    /* automatic sequence number */
	    snprintf (temp, DATEBUF_LEN, "%04d", 
		      get_next_sequence_number (newfile));
	    strncat (newfile, temp, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case 'T':
	    /* title */
	    if (!ti) goto illegal_pattern;
	    strncat (newfile, stripped_title, MAX_FILEBASELEN-nfi);
	    nfi = strlen (newfile);
	    opi+=2;
	    continue;
	case '\0':
	    /* The pattern ends in '%', but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = '\0';
	    done = 1;
	    break;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9': 
	    {
		/* Get integer */
		int ai = 0;
		char ascii_buf[7];      /* max 6 chars */
		while (isdigit (pat[opi+1+ai]) && ai < 6) {
		    ascii_buf[ai] = pat[opi+1+ai];
		    ai ++;
		}
		ascii_buf[ai] = '\0';
		/* If we got a q, get starting number */
		if (pat[opi+1+ai] == 'q') {
		    if (m_count == -1) {
			m_count = atoi(ascii_buf);
		    }
		    snprintf (temp, DATEBUF_LEN, "%04d", m_count);
		    strncat (newfile, temp, MAX_FILEBASELEN-nfi);
		    nfi = strlen (newfile);
		    opi+=ai+2;
		    continue;
		}
		/* Otherwise, no 'q', so drop through to default case */
	    }
	default:
	illegal_pattern:
	    /* Illegal pattern, but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = '\0';
	    continue;
	}
    }

    /* Pop on the extension */
    strcat (newfile, extension);
}

error_code
filelib_start (TRACK_INFO* ti)
{
    char newfile[TEMP_STR_LEN];
    char fnbase[TEMP_STR_LEN];
    char fnbase1[TEMP_STR_LEN];
    char fnbase2[TEMP_STR_LEN];

    if (!m_do_individual_tracks) return SR_SUCCESS;

    close_file(&m_file);

    /* Compose and trim filename (not including directory) */
    sprintf (fnbase1, "%s - %s", ti->artist, ti->title);
    debug_printf ("Composed filename: %s\n", fnbase1);
    trim_filename (fnbase1, fnbase);
    debug_printf ("Trimmed filename: %s\n", fnbase);
    sprintf (newfile, "%s%s%s", m_incomplete_directory, fnbase, m_extension);
    if (m_keep_incomplete) {
	int n = 1;
	char oldfilename[TEMP_STR_LEN];
	char oldfile[TEMP_STR_LEN];
	strcpy(oldfilename, fnbase);
	sprintf(oldfile, "%s%s%s", m_incomplete_directory, 
		fnbase, m_extension);
	strcpy (fnbase1, fnbase);
	while(file_exists(oldfile)) {
	    sprintf(fnbase1, "(%d)%s", n, fnbase);
	    trim_filename (fnbase1, fnbase2);
	    sprintf(oldfile, "%s%s%s", m_incomplete_directory,
		    fnbase2, m_extension);
	    n++;
	}
	if (strcmp(newfile, oldfile) != 0) {
	    MoveFile(newfile, oldfile);
	}
    }
    strcpy (m_incomplete_filename, newfile);
    return filelib_open_for_write(&m_file, newfile);
}

static long
get_file_size (char *filename)
{
    long len;
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;

    if (fseek (fp, 0, SEEK_END)) {
	fclose(fp);
	return 0;
    }

    len = ftell (fp);
    if (len < 0) {
	fclose(fp);
	return 0;
    }

    fclose (fp);
    return len;
}

/*
 * Added by Daniel Lord 29.06.2005 to only overwrite files with better 
 * captures, modified by GCS to get file size from file system 
 */
static BOOL
new_file_is_better (char *oldfile, char *newfile)
{
    long oldfilesize=0;
    long newfilesize=0;

#if defined (commentout)
    test_file=open(oldfile, O_RDWR | O_EXCL);
    oldfilesize=FileSize(test_file);
#endif

    oldfilesize = get_file_size (oldfile);
    newfilesize = get_file_size (newfile);
    
    /*
     * simple size check for now. Newfile should have at least 1Meg. Else it's
     * not very usefull most of the time.
     */
    /* GCS: This isn't quite true for low bitrate streams.  */
#if defined (commentout)
    if (newfilesize <= 524288) {
	debug_printf("NFB: newfile smaller as 524288\n");
	return FALSE;
    }
#endif

    if (oldfilesize == -1) {
	/* make sure we get the file in case of errors */
	debug_printf("NFB: could not get old filesize\n");
	return TRUE;
    }

    if (oldfilesize == newfilesize) {
	debug_printf("NFB: Size Match\n");
	return FALSE;
    }

    if (newfilesize < oldfilesize) {
	debug_printf("NFB:newfile bigger as oldfile\n");
	return FALSE;
    }

    debug_printf ("NFB:oldfilesize = %li, newfilesize = %li, "
		  "overwriting file\n", oldfilesize, newfilesize);
    return TRUE;
}
 

// Moves the file from incomplete to complete directory
// fullpath is an output parameter
error_code
filelib_end (TRACK_INFO* ti, 
	     enum OverwriteOpt overwrite,
	     BOOL truncate_dup, 
	     char *fullpath)
{
    BOOL ok_to_write = TRUE;
    char newfile[TEMP_STR_LEN];

    if (!m_do_individual_tracks) return SR_SUCCESS;

    close_file (&m_file);

    /* Construct filename for completed file */
    parse_and_subst_pat (newfile, ti, m_output_directory, 
			 m_output_pattern, m_extension);
    debug_printf ("Final output pattern:%s\n", newfile);

    /* Build up the output directory */
    mkdir_recursive (newfile, 0);

    // If we are over writing existing tracks
    switch (overwrite) {
    case OVERWRITE_ALWAYS:
	ok_to_write = TRUE;
	break;
    case OVERWRITE_NEVER:
	if (file_exists (newfile)) {
	    ok_to_write = FALSE;
	} else {
	    ok_to_write = TRUE;
	}
	break;
    case OVERWRITE_LARGER:
    default:
	/* Smart overwriting -- only overwrite if new file is bigger */
	ok_to_write = new_file_is_better (newfile, m_incomplete_filename);
	break;
    }

    if (ok_to_write) {
	if (file_exists (newfile)) {
	    DeleteFile (newfile);
	}
	MoveFile (m_incomplete_filename, newfile);
    } else {
	if (truncate_dup && file_exists(m_incomplete_filename)) {
	    TruncateFile(m_incomplete_filename);
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
filelib_write (FHANDLE fp, char *buf, u_long size)
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

static error_code
filelib_open_showfiles ()
{
    int rc;
    char cue_buf[1024];

    parse_and_subst_pat (m_show_name, 0, m_showfile_directory, 
			 m_showfile_pattern, m_extension);
    parse_and_subst_pat (m_cue_name, 0, m_showfile_directory, 
			 m_showfile_pattern, ".cue");

    rc = filelib_open_for_write (&m_cue_file, m_cue_name);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }
    /* Write cue header here */
    rc = snprintf(cue_buf,1024,"FILE \"%s\" MP3\n",m_show_name);
    rc = filelib_write(m_cue_file,cue_buf,rc);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }
    rc = filelib_open_for_write (&m_show_file, m_show_name);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }
    return rc;
}

error_code
filelib_write_show(char *buf, u_long size)
{
    error_code rc;
    if (!m_do_show) {
	return SR_SUCCESS;
    }
    rc = filelib_write (m_show_file, buf, size);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
    }
    return rc;
}

void
filelib_shutdown()
{
    close_files();
}

#if defined (commentout)
error_code
filelib_remove(char *filename)
{
    char delfile[SR_MAX_PATH];
	
    sprintf(delfile, m_filename_format, m_output_directory, filename, m_extension);
    if (!DeleteFile(delfile))
	return SR_ERROR_FAILED_TO_MOVE_FILE;

    return SR_SUCCESS;
}
#endif

/* GCS: This should get only the name, not the directory */
static void
trim_filename(char *filename, char* out)
{
    long maxlen = m_max_filename_length;
    strncpy(out, filename, MAX_TRACK_LEN);
    strip_invalid_chars(out);
    out[maxlen-4] = '\0';	// -4 for ".mp3"
}

#if defined (commentout)
static void
trim_mp3_suffix (char *filename, char* out)
{
    char* suffix_ptr;
    strncpy (out, filename, SR_MAX_PATH);
    suffix_ptr = out + strlen(out) - 4;  // -4 for ".mp3"
    if (strcmp (suffix_ptr, m_extension) == 0) {
	*suffix_ptr = 0;
    }
}
#endif

static void
trim_mp3_suffix (char *filename)
{
    char* suffix_ptr;
    suffix_ptr = filename + strlen(filename) - 4;  // -4 for ".mp3"
    if (strcmp (suffix_ptr, m_extension) == 0) {
	*suffix_ptr = 0;
    }
}

static int
get_next_sequence_number (char* fn_base)
{
    int di = 0;
    int edi = 0;
    int seq;
    char dir_name[SR_MAX_PATH];
    char fn_prefix[SR_MAX_PATH];
#if defined (WIN32)
    DIR* dp;
    struct dirent* de;
#else
    DIR* dp;
    struct dirent* de;
#endif

    /* Get directory from fn_base */
    while (fn_base[di]) {
	if (ISSLASH(fn_base[di])) {
	    edi = di;
	}
	di++;
    }
    strncpy (dir_name, fn_base, edi);
    dir_name[edi] = '\0';

    /* Get fn prefix from fn_base */
    fn_prefix[0] = '\0';
    strcpy (fn_prefix, &fn_base[edi+1]);

#if defined (WIN32)
    /* Look through directory for a filenames that match prefix */
    debug_printf ("Trying to opendir: %s\n", dir_name);
    if ((dp = opendir (dir_name)) == 0) {
	return 0;
    }
    debug_printf ("dir:%s\nprefix:%s\n", dir_name, fn_prefix);
    seq = 0;
    while ((de = readdir (dp)) != 0) {
	debug_printf ("Checking file for sequence number: %s\n", de->d_name);
	if (strncmp(de->d_name, fn_prefix, strlen(fn_prefix)) == 0) {
	    debug_printf ("Prefix match\n", de->d_name);
	    if (isdigit(de->d_name[strlen(fn_prefix)])) {
		int this_seq = atoi(&de->d_name[strlen(fn_prefix)]);
		debug_printf ("Digit match:%c,%d\n", 
			      de->d_name[strlen(fn_prefix)], this_seq);
		if (seq <= this_seq) {
		    seq = this_seq + 1;
		}
	    }
	}
    }
    closedir (dp);
    debug_printf ("Final sequence number found: %d\n",seq);
    return seq;
#else
    /* Look through directory for a filenames that match prefix */
    debug_printf ("Trying to opendir: %s\n", dir_name);
    if ((dp = opendir (dir_name)) == 0) {
	return 0;
    }
    debug_printf ("dir:%s\nprefix:%s\n", dir_name, fn_prefix);
    seq = 0;
    while ((de = readdir (dp)) != 0) {
	debug_printf ("Checking file for sequence number: %s\n", de->d_name);
	if (strncmp(de->d_name, fn_prefix, strlen(fn_prefix)) == 0) {
	    debug_printf ("Prefix match\n", de->d_name);
	    if (isdigit(de->d_name[strlen(fn_prefix)])) {
		int this_seq = atoi(&de->d_name[strlen(fn_prefix)]);
		debug_printf ("Digit match:%c,%d\n", 
			      de->d_name[strlen(fn_prefix)], this_seq);
		if (seq <= this_seq) {
		    seq = this_seq + 1;
		}
	    }
	}
    }
    closedir (dp);
    debug_printf ("Final sequence number found: %d\n",seq);
    return seq;
#endif
}
