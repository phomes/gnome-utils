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

	GttProject *prj;
};

/* ============================================================== */

static GttGhtml *ghtml_global_hack = NULL;   /* seems like guile screwed the pooch */

/* ============================================================== */
/* a simple, hard-coded version of show_table */

static void
do_show_journal (GttGhtml *ghtml, GttProject*prj)
{
	GList *node;
	char *p;
	char prn[2000];

	p = prn;
	p += sprintf (p, "<table border=1><tr><th colspan=4>%s\n"
		"<tr><th>&nbsp;<th>%s<th>%s<th>%s",
		_("Memo"), _("Start"), _("Stop"), _("Elapsed"));

	(ghtml->write_stream) (ghtml, prn, p-prn, ghtml->user_data);

	for (node = gtt_project_get_tasks(prj); node; node=node->next)
	{
		const char *pp;
		int prt_date = 1;
		time_t prev_stop = 0;
		GList *in;
		GttTask *tsk = node->data;
		
		p = prn;
		p = stpcpy (p, "<tr><td colspan=4>"
			"<a href=\"gtt:task:");
		p += sprintf (p, "%p\">", tsk);

		pp = gtt_task_get_memo(tsk);
		if (!pp || !pp[0]) pp = _("(empty)");
		p = stpcpy (p, pp);
		p = stpcpy (p, "</a>");

		(ghtml->write_stream) (ghtml, prn, p-prn, ghtml->user_data);

		
		for (in=gtt_task_get_intervals(tsk); in; in=in->next)
		{
			size_t len;
			GttInterval *ivl = in->data;
			time_t start, stop, elapsed;
			start = gtt_interval_get_start (ivl);
			stop = gtt_interval_get_stop (ivl);
			elapsed = stop - start;
			
			p = prn;
			p = stpcpy (p, 
				"<tr><td>&nbsp;&nbsp;&nbsp;"
				"<td align=right>&nbsp;&nbsp;"
				"<a href=\"gtt:interval:");
			p += sprintf (p, "%p\">", ivl);

			/* print hour only or date too? */
			if (0 != prev_stop) {
				prt_date = is_same_day(start, prev_stop);
			}
			if (prt_date) {
				p = print_date_time (p, 100, start);
			} else {
				p = print_time (p, 100, start);
			}

			/* print hour only or date too? */
			prt_date = is_same_day(start, stop);
			p = stpcpy (p, 
				"</a>&nbsp;&nbsp;"
				"<td>&nbsp;&nbsp;"
				"<a href=\"gtt:interval:");
			p += sprintf (p, "%p\">", ivl);
			if (prt_date) {
				p = print_date_time (p, 70, stop);
			} else {
				p = print_time (p, 70, stop);
			}

			prev_stop = stop;

			p = stpcpy (p, "</a>&nbsp;&nbsp;<td>&nbsp;&nbsp;");
			p = print_hours_elapsed (p, 40, elapsed, TRUE);
			p = stpcpy (p, "&nbsp;&nbsp;");
			len = p - prn;
			(ghtml->write_stream) (ghtml, prn, len, ghtml->user_data);
		}

	}
	
	p = "</table>";
	(ghtml->write_stream) (ghtml, p, strlen(p), ghtml->user_data);
}

/* ============================================================== */

static SCM 
gtt_hello (void)
{
	GttGhtml *ghtml = ghtml_global_hack;
	char *p;

	p = "Hello World!";
	(ghtml->write_stream) (ghtml, p, strlen(p), ghtml->user_data);

	return SCM_UNSPECIFIED;
}

/* ============================================================== */

static SCM 
project_title (void)
{
	GttGhtml *ghtml = ghtml_global_hack;
	const char *p;

	p = gtt_project_get_title (ghtml->prj);
	(ghtml->write_stream) (ghtml, p, strlen(p), ghtml->user_data);

	return SCM_UNSPECIFIED;
}

/* ============================================================== */

static SCM 
project_desc (void)
{
	GttGhtml *ghtml = ghtml_global_hack;
	const char *p;

	p = gtt_project_get_desc (ghtml->prj);
	(ghtml->write_stream) (ghtml, p, strlen(p), ghtml->user_data);

	return SCM_UNSPECIFIED;
}

/* ============================================================== */

static SCM 
show_journal (void)
{
	GttGhtml *ghtml = ghtml_global_hack;

	do_show_journal (ghtml, ghtml->prj);
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
	ghtml->prj = prj;
	
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
	gh_new_procedure("gtt-project-title",   project_title,   0, 0, 0);
	gh_new_procedure("gtt-project-desc",   project_desc,   0, 0, 0);
	gh_new_procedure("gtt-basic-journal",   show_journal,   0, 0, 0);
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
	p->prj = NULL;

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
