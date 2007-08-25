/*	----------------------------------------------------------------------

Copyright (C) 1998	Cesar Miquel  (miquel@df.uba.ar)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

---------------------------------------------------------------------- */


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include "logview.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "monitor.h"
#include "misc.h"
#include "logview-debug.h"

#define CHECK_FILE_PERIOD	10000

void
monitor_stop (Log *log)
{
	GnomeVFSMonitorHandle *mon_handle;
	LV_MARK;
	g_assert (LOGVIEW_IS_LOG (log));

	g_object_get (G_OBJECT (log),
		      "monitor-handle", &mon_handle, NULL);
	if (mon_handle != NULL) {
		gnome_vfs_monitor_cancel (mon_handle);
		g_object_set (G_OBJECT (log),
			      "monitor-handle", NULL, NULL);
	}
	g_object_set (G_OBJECT (log),
		      "monitoring", FALSE, NULL);
	g_object_unref (log);
}

static void
monitor_callback (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri,
		  const gchar *info_uri, GnomeVFSMonitorEventType event_type,
		  gpointer data)
{
	LogviewWindow *logview;
	Log *log = data;
	
	LV_MARK;
	g_assert (LOGVIEW_IS_LOG (log));
	
	g_object_get (G_OBJECT (log),
		      "window", &logview,
		      NULL);
	loglist_bold_log (LOG_LIST (logview->loglist), log);
	if (logview->curlog == log)
		logview_repaint (logview);
}

/* ----------------------------------------------------------------------
NAME:	monitor_log_was_modified
DESCRIPTION: Returns true if modified flag in log changed. It also
changes the modified flag. This function is called in unexpected time,
so the passed in log must be referenced. And only unreferenced when timer
is invalid.
---------------------------------------------------------------------- */

static gboolean
monitor_log_was_modified (Log *log)
{
	gboolean monitoring;
	LV_MARK;
	g_assert (LOGVIEW_IS_LOG (log));
	g_object_get (G_OBJECT (log), "monitoring", &monitoring, NULL);
	if (! monitoring) {
		g_object_unref (log);
		return FALSE;
	}
	if (log_has_been_modified (log)) {
		monitor_callback(NULL, NULL, NULL, 0, log);
	}
	return TRUE;
}

void
monitor_start (Log *log)
{
	GnomeVFSResult result;
	gchar *first = NULL, *second = NULL;
	gboolean monitorable;
	gchar *name;
	GnomeVFSMonitorHandle *mon_handle;

	LV_MARK;
	g_assert (LOGVIEW_IS_LOG (log));
	
	g_object_ref (log);
	g_object_get (G_OBJECT (log),
		      "monitorable", &monitorable,
		      "monitor-handle", &mon_handle,
		      "path", &name,
		      NULL);
	if (!monitorable) {
		g_free (name);
		g_object_unref (log);
		return;
	}
	
	result = gnome_vfs_monitor_add (&mon_handle, name,
					GNOME_VFS_MONITOR_FILE,
					monitor_callback, log);
	if (result != GNOME_VFS_OK) {
		if (result == GNOME_VFS_ERROR_NOT_SUPPORTED) {
			mon_handle = NULL;
			g_timeout_add (CHECK_FILE_PERIOD,
				       (GSourceFunc) monitor_log_was_modified,
				       g_object_ref(log));
		}
		else {
			goto gnome_vfs_err;
		}
	}
	if (mon_handle != NULL) {
		g_object_set (G_OBJECT (log),
			      "monitor-handle", mon_handle, NULL);
	}

	g_object_set (G_OBJECT (log),
		      "monitoring", TRUE, NULL);
	g_free (name);
	return;

gnome_vfs_err:
	first = g_strdup (_("Gnome-VFS error."));
	second = g_strdup_printf ("%s, (%d): %s",
				  name,
				  result,
				  gnome_vfs_result_to_string (result));
	error_dialog_show (NULL, first, second);
	g_free (first);
	g_free (second);
	g_free (name);
	g_object_unref (log);
}
