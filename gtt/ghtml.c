/*   Generate gtt-parsed html output for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#define _GNU_SOURCE
#include <guile/gh.h>
#include <stdio.h>
#include <string.h>

#include "gtt.h"
#include "ghtml.h"
#include "proj.h"
#include "util.h"


struct gtt_ghtml_s
{
	/* stream interface for writing */
	void (*open_stream) (GttGhtml *, gpointer);
	void (*write_stream) (GttGhtml *, const char *, size_t len, gpointer);
	void (*close_stream) (GttGhtml *, gpointer);
	void (*error) (GttGhtml *, int errcode, const char * msg, gpointer);
	gpointer user_data;
};

/* ============================================================== */

static GttGhtml *ghtml_global_hack = NULL;   /* seems like guile screwed the pooch */

/* ============================================================== */

static
SCM gtt_hello (void)
{
	GttGhtml *ghtml = ghtml_global_hack;
	char *p;

	p = "Hello World!<br><br>";
	(ghtml->write_stream) (ghtml, p, strlen(p), ghtml->user_data);

	return SCM_UNSPECIFIED;
}

/* ============================================================== */

void
gtt_ghtml_display (GttGhtml *ghtml, const char *filepath,
                   GttProject *prj)
{
	FILE *ph;

	if (!ghtml) return;

	if (!filepath)
	{
		(ghtml->error) (ghtml, 404, NULL, ghtml->user_data);
		return;
	}

	/* try to get the ghtml file ... */
	ph = fopen (filepath, "r");
	if (!ph)
	{
		(ghtml->error) (ghtml, 404, filepath, ghtml->user_data);
		return;
	}
	
	/* ugh. gag. choke. puke. */
	ghtml_global_hack = ghtml;

	/* Now open the output stream for writing */
	(ghtml->open_stream) (ghtml, ghtml->user_data);

	while (!feof (ph))
	{
#define BUFF_SIZE 4000
		size_t nr;
		char *start, *end, *scmstart, *scmend, *comstart, *comend;
		char buff[BUFF_SIZE+1];
		nr = fread (buff, 1, BUFF_SIZE, ph);
		if (0 >= nr) break;  /* EOF I presume */
		buff[nr] = 0x0;
		
		start = buff;
		while (start)
		{
			scmstart = NULL;
			scmend = NULL;
			
			/* look for scheme markup */
			end = strstr (start, "<?scm");

			/* look for comments, and blow past them. */
			comstart = strstr (start, "<!--");
			if (comstart && comstart < end)
			{
				comend = strstr (comstart, "-->");
				if (comend)
				{
					nr = comend - start;
					end = comend;
				}
				else
				{
					nr = strlen (start);
					end = NULL;
				}
				/* write everything that we got before the markup */
				(ghtml->write_stream) (ghtml, start, nr, ghtml->user_data);
				start = end;
				continue;
			}

			/* look for  termination of scm markup */
			if (end)
			{
				nr = end - start;
				*end = 0x0;
				scmstart = end+5;
				end = strstr (scmstart, "?>");
				if (end)
				{
					*end = 0;
					end += 2;
				}
			}
			else
			{
				nr = strlen (start);
			}
			
			/* write everything that we got before the markup */
			(ghtml->write_stream) (ghtml, start, nr, ghtml->user_data);

			/* if there is markup, then dispatch */
			if (scmstart)
			{
				gh_eval_str_with_standard_handler (scmstart);
			}
			start = end;
		}
			
	}
	fclose (ph);

	(ghtml->close_stream) (ghtml, ghtml->user_data);

}

/* ============================================================== */

static int is_inited = 0;

static void
register_procs (void)
{
	gh_new_procedure("gtt-hello",   gtt_hello,   0, 0, 0);
}


/* ============================================================== */

GttGhtml *
gtt_ghtml_new (void)
{
	GttGhtml *p;

	if (!is_inited)
	{
		is_inited = 1;
		register_procs();
	}

	p = g_new0 (GttGhtml, 1);

	return p;
}

void 
gtt_ghtml_destroy (GttGhtml *p)
{
	if (!p) return;
	g_free (p);
}

void gtt_ghtml_set_stream (GttGhtml *p, gpointer ud,
                                        GttGhtmlOpenStream op, 
                                        GttGhtmlWriteStream wr,
                                        GttGhtmlCloseStream cl, 
                                        GttGhtmlError er)
{
	if (!p) return;
	p->user_data = ud;
	p->open_stream = op;
	p->write_stream = wr;
	p->close_stream = cl;
	p->error = er;
}

/* ===================== END OF FILE ==============================  */
