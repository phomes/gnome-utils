/* userprefs.c - logview user preferences handling
 *
 * Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)
 * Copyright (C) 2004  Vincent Noel
 * Copyright (C) 2006  Emmanuele Bassi
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>
#include <sys/stat.h>
#include "userprefs.h"
#include "logview.h"
#include "logview-plugin-manager.h"
#include "logview-debug.h"

#define LOGVIEW_DEFAULT_HEIGHT 400
#define LOGVIEW_DEFAULT_WIDTH 600

/* logview settings */
#define GCONF_DIR 		"/apps/gnome-system-log"
#define GCONF_WIDTH_KEY 	GCONF_DIR "/width"
#define GCONF_HEIGHT_KEY 	GCONF_DIR "/height"
#define GCONF_LOGFILE 		GCONF_DIR "/logfile"
#define GCONF_LOGFILES 		GCONF_DIR "/logfiles"
#define GCONF_FONTSIZE_KEY 	GCONF_DIR "/fontsize"

/* desktop-wide settings */
#define GCONF_MONOSPACE_FONT_NAME "/desktop/gnome/interface/monospace_font_name"
#define GCONF_MENUS_HAVE_TEAROFF  "/desktop/gnome/interface/menus_have_tearoff"

static GConfClient *gconf_client = NULL;
static UserPrefs *prefs;

static void
prefs_create_defaults (UserPrefs *p)
{
	GSList* idx;

	LV_MARK;
	g_assert (p->logs == NULL);
	p->logfile = NULL;
	p->logs = pluginmgr_get_all_logs ();

	for (idx = p->logs; idx != NULL; idx = idx->next) {
		LV_ERR ("add|log: %s", (gchar*) idx->data);
	}
}

static UserPrefs *
prefs_load (void)
{
	gchar *logfile;
	int width, height, fontsize;
	UserPrefs *p;
	GError *err;

	g_assert (gconf_client != NULL);

	p = g_new0 (UserPrefs, 1);

	err = NULL;
	p->logs = gconf_client_get_list (gconf_client,
					 GCONF_LOGFILES,
					 GCONF_VALUE_STRING,
					 &err);
	if (p->logs == NULL)
		prefs_create_defaults (p);
	if (p->logs)
		p->logfile = p->logs->data;
	
	logfile = NULL;
	logfile = gconf_client_get_string (gconf_client, GCONF_LOGFILE, NULL);
	if (logfile && logfile[0] != '\0') {
		GSList *iter;
		
		for (iter = p->logs; iter != NULL; 
		     iter = g_slist_next (iter)) {
			if (g_ascii_strncasecmp (iter->data, 
						 logfile, 255) == 0) {
				p->logfile = iter->data;
				break;
			}
		}
		g_free (logfile);
	}

	width = gconf_client_get_int (gconf_client, GCONF_WIDTH_KEY, NULL);
	height = gconf_client_get_int (gconf_client, GCONF_HEIGHT_KEY, NULL);
	fontsize = gconf_client_get_int (gconf_client, GCONF_FONTSIZE_KEY, NULL);

	p->width = (width == 0 ? LOGVIEW_DEFAULT_WIDTH : width);
	p->height = (height == 0 ? LOGVIEW_DEFAULT_HEIGHT : height);
	p->fontsize = fontsize;
	p->logfile = g_strdup (p->logfile);

	return p;
}

static void
prefs_monospace_font_changed (GConfClient *client,
                              guint id,
                              GConfEntry *entry,
                              gpointer data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (data);

	if (entry->value && (entry->value->type == GCONF_VALUE_STRING)) {
		const gchar *monospace_font_name;

		monospace_font_name = gconf_value_get_string (entry->value);
		logview_set_font (logview, monospace_font_name);
	}
}

static void
prefs_menus_have_tearoff_changed (GConfClient *client,
                                  guint id,
                                  GConfEntry *entry,
                                  gpointer data)
{
	LogviewWindow *logview = LOGVIEW_WINDOW (data);

	if (entry->value && (entry->value->type == GCONF_VALUE_BOOL)) {
		gboolean add_tearoffs;

		add_tearoffs = gconf_value_get_bool (entry->value);
		gtk_ui_manager_set_add_tearoffs (logview->ui_manager,
						 add_tearoffs);
	}
}

gchar *
prefs_get_monospace (void)
{
	return (gconf_client_get_string (gconf_client, GCONF_MONOSPACE_FONT_NAME, NULL));
}

gboolean
prefs_get_have_tearoff (void)
{
	return (gconf_client_get_bool (gconf_client, GCONF_MENUS_HAVE_TEAROFF, NULL));
}

GSList *
prefs_get_logs (void)
{
	return prefs->logs;
}

gchar *
prefs_get_active_log (void) 
{
	return prefs->logfile;
}

int
prefs_get_width (void)
{
	return prefs->width;
}

int
prefs_get_height (void)
{
	return prefs->height;
}

void
prefs_free_loglist ()
{
	GSList *idx;
	if (prefs->logs) {
		for (idx = prefs->logs; idx; idx = g_slist_next(idx)) {
			g_free ((gchar*)idx->data);
		}
		g_slist_free (prefs->logs);
		prefs->logs = NULL;
	}
	if (prefs->logfile) {
		g_free (prefs->logfile);
		prefs->logfile = NULL;
	}
}

void
prefs_store_log (const gchar *name)
{
	LV_INFO ("[Prefs Store] %s", name);
	if (name && name[0] != '\0')
		prefs->logs = g_slist_append (prefs->logs, g_strdup (name));
}

void
prefs_store_active_log (const gchar *name)
{
	/* name can be NULL if no active log */
	if (prefs->logfile)
		g_free (prefs->logfile);
	if (name)
		prefs->logfile = g_strdup(name);
	else
		prefs->logfile = NULL;
}

void
prefs_store_fontsize (int fontsize)
{
	prefs->fontsize = fontsize;
}

void
prefs_save (void)
{
	g_assert (gconf_client != NULL);

	if (prefs->logfile) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_LOGFILE, NULL))
			gconf_client_set_string (gconf_client,
						 GCONF_LOGFILE,
						 prefs->logfile,
						 NULL);
	}

	if (gconf_client_key_is_writable (gconf_client, GCONF_LOGFILES, NULL))
		gconf_client_set_list (gconf_client,
				       GCONF_LOGFILES,
				       GCONF_VALUE_STRING,
				       prefs->logs,
				       NULL);

	if (prefs->width > 0 && prefs->height > 0) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_WIDTH_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_WIDTH_KEY,
					      prefs->width,
					      NULL);
		
		if (gconf_client_key_is_writable (gconf_client, GCONF_HEIGHT_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_HEIGHT_KEY,
					      prefs->height,
					      NULL);
	}

	if (prefs->fontsize > 0) {
		if (gconf_client_key_is_writable (gconf_client, GCONF_FONTSIZE_KEY, NULL))
			gconf_client_set_int (gconf_client,
					      GCONF_FONTSIZE_KEY,
					      prefs->fontsize,
					      NULL);
	}
}

void
prefs_store_window_size (GtkWidget *window)
{
	gint width, height;

	g_return_if_fail (GTK_IS_WINDOW (window));
      
	gtk_window_get_size (GTK_WINDOW(window), &width, &height);
	if (width > 0 && height > 0) {
		prefs->width = width;
		prefs->height = height;
	}
}

void 
prefs_connect (LogviewWindow *logview)
{
	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	g_assert (gconf_client != NULL);

	gconf_client_notify_add (gconf_client,
				 GCONF_MONOSPACE_FONT_NAME,
				 (GConfClientNotifyFunc) prefs_monospace_font_changed,
				 logview, NULL,
				 NULL);
	gconf_client_notify_add (gconf_client,
				 GCONF_MENUS_HAVE_TEAROFF,
				 (GConfClientNotifyFunc) prefs_menus_have_tearoff_changed,
				 logview, NULL,
				 NULL);
}

void
prefs_init (void)
{
	if (!gconf_client) {
		gconf_client = gconf_client_get_default ();

		prefs = prefs_load ();
	}
}
