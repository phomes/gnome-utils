/*   Global Preferences for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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

#include <glade/glade.h>
#include <gnome.h>
#include <libgnome/gnome-help.h>
#include <string.h>

#include "proj.h"
#include "props-proj.h"
#include "util.h"


typedef struct _PrefsDialog 
{
	GladeXML *gtxml;
	GnomePropertyBox *dlg;
	GtkEntry *desc;
} PrefsDialog;


/* ============================================================== */

static void 
prop_set(GnomePropertyBox * pb, gint page, PrefsDialog *dlg)
{
	int ivl;
	double rate;
	gchar *str;

	if (!dlg->proj) return;

	if (0 == page)
	{
		gtt_project_freeze (dlg->proj);
		str = gtk_entry_get_text(dlg->title);
		if (str && str[0]) 
		{
			gtt_project_set_title(dlg->proj, str);
		} 
		else 
		{
			gtt_project_set_title(dlg->proj, _("empty"));
			gtk_entry_set_text(dlg->title, _("empty"));
		}
	
		gtt_project_set_desc(dlg->proj, gtk_entry_get_text(dlg->desc));
		gtt_project_set_notes(dlg->proj, xxxgtk_text_get_text(dlg->notes));
		gtt_project_thaw (dlg->proj);
	}

	if (1 == page)
	{
		gtt_project_freeze (dlg->proj);
		rate = atof (gtk_entry_get_text(dlg->regular));
		gtt_project_set_billrate (dlg->proj, rate);
		gtt_project_thaw (dlg->proj);
	}
	
	if (2 == page)
	{
		gtt_project_freeze (dlg->proj);
		ivl = atoi (gtk_entry_get_text(dlg->minvl));
		gtt_project_set_min_interval (dlg->proj, ivl);
		gtt_project_thaw (dlg->proj);
	}
}


/* ============================================================== */


static void 
do_set_project(GttProject *proj, PrefsDialog *dlg)
{
	char buff[132];

	if (!dlg) return;

/* hack alert -- fixme -- we don't really need to do this work
 * if the thing aint visible. we should check for visibility and defer ...
 */
	if (!proj) 
	{
		/* We null these out, because old values may be left
		 * over from an earlier project */
		dlg->proj = NULL;
		gtk_entry_set_text(dlg->title, "");
		gtk_entry_set_text(dlg->desc, "");
		return;
	}

	if (dlg->proj == proj) return;

	dlg->proj = proj;

	gtk_entry_set_text(dlg->title, gtt_project_get_title(proj));
	gtk_entry_set_text(dlg->desc, gtt_project_get_desc(proj));

	g_snprintf (buff, 132, "%d", gtt_project_get_min_interval(proj));
	gtk_entry_set_text(dlg->minvl, buff);

	/* set to unmodified as it reflects the current state of the project */
	gnome_property_box_set_modified(GNOME_PROPERTY_BOX(dlg->dlg),
					FALSE);
}

/* ============================================================== */

static PrefsDialog *
prefs_dialog_new (void)
{
        PrefsDialog *dlg;
	GladeXML *gtxml;
	GtkWidget *e;
        static GnomeHelpMenuEntry help_entry = { NULL, "index.html#PROP" };

	dlg = g_malloc(sizeof(PrefsDialog));

	gtxml = glade_xml_new ("glade/project_properties.glade", "Project Properties");
	dlg->gtxml = gtxml;

	dlg->dlg = GNOME_PROPERTY_BOX (glade_xml_get_widget (gtxml,  "Project Properties"));

	help_entry.name = gnome_app_id;
	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "help",
			   GTK_SIGNAL_FUNC(gnome_help_pbox_display),
			   &help_entry);

	gtk_signal_connect(GTK_OBJECT(dlg->dlg), "apply",
			   GTK_SIGNAL_FUNC(prop_set), dlg);

	/* ------------------------------------------------------ */
	/* grab the various entry boxes and hook them up */
	e = glade_xml_get_widget (gtxml, "title box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->title = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "desc box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->desc = GTK_ENTRY(e);

	e = glade_xml_get_widget (gtxml, "notes box");
	gtk_signal_connect_object(GTK_OBJECT(e), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed), 
				  GTK_OBJECT(dlg->dlg));
	dlg->gap = GTK_ENTRY(e);

	gnome_dialog_close_hides(GNOME_DIALOG(dlg->dlg), TRUE);
/*
	gnome_dialog_set_parent(GNOME_DIALOG(dlg->dlg), GTK_WINDOW(window));

*/
	return dlg;
}


/* ============================================================== */

static PrefsDialog *dlog = NULL;

void 
prefs_dialog_show(GttProject *proj)
{
	if (!dlog) dlog = prop_dialog_new();
 
	do_set_project(proj, dlog);
	gtk_widget_show(GTK_WIDGET(dlog->dlg));
}

/* ==================== END OF FILE ============================= */
