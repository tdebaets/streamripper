/* parse.c
 * metadata parsing routines
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
#include "srconfig.h"
#include "debug.h"
#include "types.h"
#include "util.h"
#include "regex.h"

/*****************************************************************************
 * Public functions
 *****************************************************************************/

/*****************************************************************************
 * Private global variables
 *****************************************************************************/
#define MAX_RULE_SIZE                2048
#define MAX_SUBMATCHES                 24

#define PARSERULE_CMD_MATCH          0x01
#define PARSERULE_CMD_SUBST          0x02

#define PARSERULE_SKIP               0x01
#define PARSERULE_GLOBAL             0x02
#define PARSERULE_ICASE              0x04


struct parse_rule {
    int cmd;
    int flags;
    int artist_idx;
    int title_idx;
    int album_idx;
    int trackno_idx;
    regex_t* reg;
    char* match;
    char* subst;
};
typedef struct parse_rule Parse_Rule;

static Parse_Rule m_default_rule_list[] = {
    { PARSERULE_CMD_MATCH, 
	PARSERULE_SKIP,
	0, 0, 0, 0,
	0, 
	"^A suivre:",
	""
    },
    { PARSERULE_CMD_SUBST, 
	PARSERULE_ICASE,
	0, 0, 0, 0,
	0, 
	"[[:space:]]*-?[[:space:]]*mp3pro$",
	""
    },
    { PARSERULE_CMD_MATCH, 
	PARSERULE_ICASE,
	1, 2, 0, 0,
	0, 
	"^[[:space:]]*([^-]*?)[[:space:]]*-[[:space:]]*(.*?)[[:space:]]*$",
	""
    },
    { 0x00, 
	0x00,
	0, 0, 0, 0,
	0, 
	"",
	""
    }
};
static Parse_Rule* m_global_rule_list = m_default_rule_list;

/* Returns 1 if successful, 0 if failure */
static int
compile_rule (Parse_Rule* pr, char* rule_string)
{
    int rc;
    int cflags;

    pr->reg = (regex_t*) malloc (sizeof(regex_t));
    if (!pr->reg) return 0;

    cflags = REG_EXTENDED;
    if (pr->flags & PARSERULE_ICASE) {
	cflags |= REG_ICASE;
    }
    rc = regcomp(pr->reg, rule_string, cflags);
    if (rc != 0) {
	printf ("Warning: malformed regular expression:\n%s\n",
	    rule_string);
	free(pr->reg);
	return 0;
    }
    return 1;
}

static void
use_default_rules (void)
{
    Parse_Rule* rulep;

    /* set global rule list to default */
    m_global_rule_list = m_default_rule_list;

    /* compile regular expressions */
    for (rulep = m_global_rule_list; rulep->cmd; rulep++) {
	compile_rule (rulep, rulep->match);
    }
}

static void
copy_rule_result (char* dest, char* query_string, regmatch_t* pmatch, int idx)
{
    if (idx > 0 && idx <= MAX_SUBMATCHES) {
	sr_strncpy(dest, query_string + pmatch[idx].rm_so,
	    pmatch[idx].rm_eo - pmatch[idx].rm_so + 1);
    }
}

/* Return 1 if successful, or 0 for failure */
static int
parse_flags (Parse_Rule* pr, char* flags)
{
    char flag1;

    while ((flag1 = *flags++)) {
        int* tag = 0;
	switch (flag1) {
	case 'e':
	    pr->flags |= PARSERULE_SKIP;
	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    break;
	case 'g':
	    /* GCS FIX: Not yet implemented */
	    pr->flags |= PARSERULE_GLOBAL;
	    break;
	case 'i':
	    pr->flags |= PARSERULE_ICASE;
	    break;
	case 'A':
	    tag = &pr->artist_idx;
	    break;
	case 'C':
	    tag = &pr->album_idx;
	    break;
	case 'N':
	    tag = &pr->trackno_idx;
	    break;
	case 'T':
	    tag = &pr->title_idx;
	    break;
	case 0:
	case '\n':
	case '\r':
	    return 1;
	default:
	    return 0;
	}
	if (tag) {
	    int rc;
	    int nchar;
	    int idx = 0;

	    if (pr->cmd != PARSERULE_CMD_MATCH) return 0;
	    rc = sscanf (flags, "%d%n", &idx, &nchar);
	    if (rc == 0 || idx > MAX_SUBMATCHES) {
		return 0;
	    }
	    flags += nchar;
	    *tag = idx;
	}
    }
    return 1;
}

static char* 
parse_escaped_string (char* outbuf, char* inbuf)
{
    int escaped = 0;

    while (1) {
	switch (*outbuf++ = *inbuf++) {
	case '\\':
	    escaped = !escaped;
	    break;
	case '/':
	    if (!escaped){
		*(--outbuf) = 0;
		return inbuf;
	    }
	    break;
	case '0':
	    return 0;
	    break;
	default:
	    escaped = 0;
	    break;
	}
    }
    /* Never get here */
    return 0;
}

/* This mega-function reads in the rules file, and loads 
    all the rules into the m_global_rules_list data structure */
void
init_metadata_parser (char* rules_file)
{
    FILE* fp;
    int ri;     /* Rule index */

    if (!rules_file) {
	use_default_rules ();
	return;
    }
    fp = fopen (rules_file, "r");
    if (!fp) {
	use_default_rules ();
	return;
    }

    m_global_rule_list = 0;
    ri = 0;
    while (!feof(fp)) {
	char rule_buf[MAX_RULE_SIZE];
	char match_buf[MAX_RULE_SIZE];
	char subst_buf[MAX_RULE_SIZE];
	char* rbp;
	int got_command;
	int rc;
	
	/* Allocate memory for rule */
	m_global_rule_list = realloc (m_global_rule_list,
	    (ri+1) * sizeof(Parse_Rule));
	memset (&m_global_rule_list[ri], 0, sizeof(Parse_Rule));

	/* Get next line from file */
	fgets (rule_buf,2048,fp);

	/* Skip leading whitespace */
	rbp = rule_buf;
	while (*rbp && isspace(*rbp)) rbp++;
	if (!*rbp) continue;

	/* Get command */
	got_command = 0;
	switch (*rbp++) {
	case 'm':
	    got_command = 1;
	    m_global_rule_list[ri].cmd = PARSERULE_CMD_MATCH;
	    break;
	case 's':
	    got_command = 1;
	    m_global_rule_list[ri].cmd = PARSERULE_CMD_SUBST;
	    break;
	case '#':
	    got_command = 0;
	    break;
	default:
	    got_command = 0;
	    printf ("Warning: malformed command in rules file:\n%s\n",
		rule_buf);
	    break;
	}
	if (!got_command) continue;

	/* Skip past fwd slash */
	if (*rbp++ != '/') {
	    printf ("Warning: malformed command in rules file:\n%s\n",
		rule_buf);
	    continue;
	}

	/* Parse match string */
	rbp = parse_escaped_string (match_buf, rbp);
	    if (!rbp) {
		printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
		continue;
	    }

	/* Parse subst string */
	if (m_global_rule_list[ri].cmd == PARSERULE_CMD_SUBST) {
	    rbp = parse_escaped_string (subst_buf, rbp);
	    if (!rbp) {
		printf ("Warning: malformed command in rules file:\n%s\n",
		    rule_buf);
		continue;
	    }
	}

	/* Parse flags */
	rc = parse_flags (&m_global_rule_list[ri], rbp);
	if (!rc) {
	    printf ("Warning: malformed command in rules file:\n%s\n",
		rule_buf);
	    continue;
	}

	/* Compile the rule */
	if (!compile_rule(&m_global_rule_list[ri], match_buf)) continue;

	/* Copy rule strings */
	m_global_rule_list[ri].match = strdup(match_buf);
	if (m_global_rule_list[ri].cmd == PARSERULE_CMD_SUBST) {
	    m_global_rule_list[ri].subst = strdup(subst_buf);
	}

	ri++;
    }
    /* Add null rule to the end */
    m_global_rule_list = realloc (m_global_rule_list,
	(ri+1) * sizeof(Parse_Rule));
    memset (&m_global_rule_list[ri], 0, sizeof(Parse_Rule));
}

void
parse_metadata (TRACK_INFO* ti)
{
    int eflags;
    int rc;
    char query_string[MAX_TRACK_LEN];
    Parse_Rule* rulep;

    ti->have_track_info = 0;
    ti->artist[0] = 0;
    ti->title[0] = 0;
    ti->album[0] = 0;
    if (!ti->raw_metadata[0]) {
	return;
    }

    /* Loop through rules, if we find a matching rule, then use it */
    /* For now, only default rules supported with ascii 
       regular expressions. */
    strcpy (query_string, ti->raw_metadata);
    for (rulep = m_global_rule_list; rulep->cmd; rulep++) {
	regmatch_t pmatch[MAX_SUBMATCHES+1];

	eflags = 0;
	if (rulep->cmd == PARSERULE_CMD_MATCH) {
	    if (rulep->flags & PARSERULE_SKIP) {
		rc = regexec(rulep->reg, query_string, 0, NULL, eflags);
		if (rc != 0) {
		    /* Didn't match rule. */
		    continue;
		}
		/* GCS FIX: We need to return to the 
		    caller that the metadata should be dropped. */
		ti->have_track_info = 0;
		return;
	    } else {
    		eflags = 0;
		rc = regexec(rulep->reg, query_string, MAX_SUBMATCHES+1, pmatch, eflags);
		if (rc != 0) {
		    /* Didn't match rule. */
		    continue;
		}
		copy_rule_result (ti->artist, query_string, pmatch, rulep->artist_idx);
		copy_rule_result (ti->title, query_string, pmatch, rulep->title_idx);
		copy_rule_result (ti->album, query_string, pmatch, rulep->album_idx);
		/* GCS FIX: We don't have a track no */
		ti->have_track_info = 1;
		return;
	    }
	}
	else if (rulep->cmd == PARSERULE_CMD_SUBST) {
	    char subst_string[MAX_TRACK_LEN];
	    int used, left;
	    rc = regexec(rulep->reg, query_string, 1, pmatch, eflags);
	    if (rc != 0) {
		/* Didn't match rule. */
		continue;
	    }
	    /* Update the query string and continue. */
	    strncpy(subst_string, query_string, pmatch[0].rm_so);
	    used = pmatch[0].rm_so;
	    left = MAX_TRACK_LEN - used;
	    strncpy(subst_string + used, rulep->subst, left-1);
	    used += strlen (rulep->subst);
	    left = MAX_TRACK_LEN - used;
	    sr_strncpy(subst_string + used, query_string + pmatch[0].rm_eo, left);
	    sr_strncpy(query_string, subst_string, MAX_TRACK_LEN);
	}
    }
}
