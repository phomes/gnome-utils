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
#include <stdio.h>
#include <string.h>

#include "gtt.h"
#include "phtml.h"
#include "proj.h"
#include "util.h"

typedef enum {
	NUL=0,
	START_DATIME =1,
	STOP_DATIME,
	ELAPSED,
	BILLABLE,
	BILLRATE,

} TableCol;

#define NCOL 30

struct gtt_phtml_s
{
	/* stream interface for writing */
	void (*open_stream) (GttPhtml *, gpointer);
	void (*write_stream) (GttPhtml *, const char *, size_t len, gpointer);
	void (*close_stream) (GttPhtml *, gpointer);
	void (*error) (GttPhtml *, int errcode, const char * msg, gpointer);
	gpointer user_data;

	/* parseing state */
	int ncols;
	TableCol cols[NCOL];
};

/* ============================================================== */

static void
show_table (GttPhtml *phtml, GttProject*prj, int show_links)
{
	int i;
	GList *node;
	char *p;
	char buff[2000];

	p = buff;
	p += sprintf (p, "<table border=1><tr><th colspan=%d>%s<tr>",
		phtml->ncols, _("Memo"));


	/* write out the table header */
	for (i=0; i<phtml->ncols; i++)
	{
		p = stpcpy (p, "<th>");
		switch (phtml->cols[i]) 
		{
			case START_DATIME:
				p = stpcpy (p, _("Start"));
				break;
			case STOP_DATIME:
				p = stpcpy (p, _("Stop"));
				break;
			case ELAPSED:
				p = stpcpy (p, _("Elapsed"));
				break;
			case BILLABLE:
				p = stpcpy (p, _("Billable"));
				break;
			case BILLRATE:
				p = stpcpy (p, _("Bill Rate"));
				break;
			default:
				p = stpcpy (p, _("Error - Unknown"));
		}
	}


	(phtml->write_stream) (phtml, buff, p-buff, phtml->user_data);

	for (node = gtt_project_get_tasks(prj); node; node=node->next)
	{
		const char *pp;
		time_t prev_stop = 0;
		GList *in;
		GttTask *tsk = node->data;
		
		/* write the memo message */
		p = buff;
		p += sprintf (p, "<tr><td colspan=%d>", phtml->ncols);
		if (show_links) 
		{
			p = stpcpy (p, "<a href=\"gtt:task:");
			p += sprintf (p, "%p\">", tsk);
		}

		pp = gtt_task_get_memo(tsk);
		if (!pp || !pp[0]) pp = _("(empty)");
		p = stpcpy (p, pp);
		if (show_links) p = stpcpy (p, "</a>");

		(phtml->write_stream) (phtml, buff, p-buff, phtml->user_data);
		
		/* write out intervals */
		for (in=gtt_task_get_intervals(tsk); in; in=in->next)
		{
			size_t len;
			GttInterval *ivl = in->data;
			time_t start, stop, elapsed;
			int prt_start_date = 1;
			int prt_stop_date = 1;

			/* data setup */
			start = gtt_interval_get_start (ivl);
			stop = gtt_interval_get_stop (ivl);
			elapsed = stop - start;

			/* print hour only or date too? */
			prt_stop_date = is_same_day(start, stop);
			if (0 != prev_stop) {
				prt_start_date = is_same_day(start, prev_stop);
			}
			prev_stop = stop;

			p = buff;
			p = stpcpy (p, "<tr>");
			for (i=0; i<phtml->ncols; i++)
			{

				switch (phtml->cols[i]) 
				{
	case START_DATIME:
	{
		p = stpcpy (p, "<td align=right>&nbsp;&nbsp;");
		if (show_links)
		{
			p += sprintf (p, "<a href=\"gtt:interval:%p\">", ivl);
		}
		if (prt_start_date) {
			p = print_date_time (p, 100, start);
		} else {
			p = print_time (p, 100, start);
		}
		if (show_links) p = stpcpy (p, "</a>");
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	case STOP_DATIME:
	{
		p = stpcpy (p, "<td align=right>&nbsp;&nbsp;");
		if (show_links)
		{
			p += sprintf (p, "<a href=\"gtt:interval:%p\">", ivl);
		}
		if (prt_stop_date) {
			p = print_date_time (p, 100, stop);
		} else {
			p = print_time (p, 100, stop);
		}
		if (show_links) p = stpcpy (p, "</a>");
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	case ELAPSED:
	{
		p = stpcpy (p, "<td>&nbsp;&nbsp;");
		p = print_hours_elapsed (p, 40, elapsed, TRUE);
		p = stpcpy (p, "&nbsp;&nbsp;");
		break;
	}
	case BILLABLE:
		p = stpcpy (p, "<td>");
		p = stpcpy (p, _("Billable"));
		break;
	case BILLRATE:
		p = stpcpy (p, "<td>");
		p = stpcpy (p, _("Bill Rate"));
		break;
	default:
		p = stpcpy (p, "<td>");
				}
			}

			len = p - buff;
			(phtml->write_stream) (phtml, buff, len, phtml->user_data);
		}

	}
	
	p = "</table>";
	(phtml->write_stream) (phtml, p, strlen(p), phtml->user_data);

}

/* ============================================================== */
/* a simpler, hard-coded version of show_table */

static void
show_journal (GttPhtml *phtml, GttProject*prj)
{
	GList *node;
	char *p;
	char prn[2000];

	p = prn;
	p += sprintf (p, "<table border=1><tr><th colspan=4>%s\n"
		"<tr><th>&nbsp;<th>%s<th>%s<th>%s",
		_("Memo"), _("Start"), _("Stop"), _("Elapsed"));

	(phtml->write_stream) (phtml, prn, p-prn, phtml->user_data);

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

		(phtml->write_stream) (phtml, prn, p-prn, phtml->user_data);

		
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
			(phtml->write_stream) (phtml, prn, len, phtml->user_data);
		}

	}
	
	p = "</table>";
	(phtml->write_stream) (phtml, p, strlen(p), phtml->user_data);

}

/* ============================================================== */

static void
dispatch_phtml (GttPhtml *phtml, char *tok, GttProject*prj)
{
	if (!prj) return;
	while (tok)
	{

		if (0 == strncmp (tok, "$project_title", 13))
		{
			const char * str = gtt_project_get_title(prj);
			if (!str || !str[0]) str = _("(empty)");
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}
		else
		if (0 == strncmp (tok, "$project_desc", 13))
		{
			const char * str = gtt_project_get_desc(prj);
			str = str? str : "";
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}
		else
		if (0 == strncmp (tok, "$journal", 8))
		{
			show_journal (phtml, prj);
		}
		else
		if (0 == strncmp (tok, "$table", 6))
		{
			show_table (phtml, prj, 1);
		}
		else
		if (0 == strncmp (tok, "$start_datime", 13))
		{
			phtml->cols[phtml->ncols] = START_DATIME;
			if (NCOL-1 > phtml->ncols) phtml->ncols ++;
		}
		else
		if (0 == strncmp (tok, "$stop_datime", 12))
		{
			phtml->cols[phtml->ncols] = STOP_DATIME;
			if (NCOL-1 > phtml->ncols) phtml->ncols ++;
		}
		else
		if (0 == strncmp (tok, "$elapsed", 8))
		{
			phtml->cols[phtml->ncols] = ELAPSED;
			if (NCOL-1 > phtml->ncols) phtml->ncols ++;
		}
		else
		if (0 == strncmp (tok, "$billable", 9))
		{
			phtml->cols[phtml->ncols] = BILLABLE;
			if (NCOL-1 > phtml->ncols) phtml->ncols ++;
		}
		else
		if (0 == strncmp (tok, "$billrate", 9))
		{
			phtml->cols[phtml->ncols] = BILLRATE;
			if (NCOL-1 > phtml->ncols) phtml->ncols ++;
		}
		else
		{
			const char * str;
			str = _("unknown token: >>>>");
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
			str = tok;
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
			str = "<<<<";
			(phtml->write_stream) (phtml, str, strlen(str), phtml->user_data);
		}

		/* find the next one */
		tok ++;
		tok = strchr (tok, '$');
	}
}

/* ============================================================== */

void
gtt_phtml_display (GttPhtml *phtml, const char *filepath,
                   GttProject *prj)
{
	FILE *ph;

	if (!phtml)
		
	if (!filepath)
	{
		(phtml->error) (phtml, 404, NULL, phtml->user_data);
		return;
	}

	/* try to get the phtml file ... */
	ph = fopen (filepath, "r");
	if (!ph)
	{
		(phtml->error) (phtml, 404, filepath, phtml->user_data);
		return;
	}

	/* Now open the output stream for writing */
	(phtml->open_stream) (phtml, phtml->user_data);

	while (!feof (ph))
	{
#define BUFF_SIZE 4000
		size_t nr;
		char *start, *end, *tok;
		char buff[BUFF_SIZE+1];
		nr = fread (buff, 1, BUFF_SIZE, ph);
		if (0 >= nr) break;  /* EOF I presume */
		buff[nr] = 0x0;
		
		start = buff;
		while (start)
		{
			tok = NULL;
			
			/* look for special gtt markup */
			end = strstr (start, "<?gtt");
			if (end)
			{
				nr = end - start;
				*end = 0x0;
				tok = end+5;
				end = strstr (tok, "?>");
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
			(phtml->write_stream) (phtml, start, nr, phtml->user_data);

			/* if there is markup, then dispatch */
			if (tok)
			{
				tok = strchr (tok, '$');
				dispatch_phtml (phtml, tok, prj);
			}
			start = end;
		}
			
	}
	fclose (ph);

	(phtml->close_stream) (phtml, phtml->user_data);

}

/* ============================================================== */

GttPhtml *
gtt_phtml_new (void)
{
	GttPhtml *p;

	p = g_new0 (GttPhtml, 1);
	p ->ncols = 0;

	return p;
}

void 
gtt_phtml_destroy (GttPhtml *p)
{
	if (!p) return;
	g_free (p);
}

void gtt_phtml_set_stream (GttPhtml *p, gpointer ud,
                                        GttPhtmlOpenStream op, 
                                        GttPhtmlWriteStream wr,
                                        GttPhtmlCloseStream cl, 
                                        GttPhtmlError er)
{
	if (!p) return;
	p->user_data = ud;
	p->open_stream = op;
	p->write_stream = wr;
	p->close_stream = cl;
	p->error = er;
}

/* ===================== END OF FILE ==============================  */
