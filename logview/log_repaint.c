
/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "logview.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"
#include "calendar.h"
#include "logview-debug.h"

static gboolean busy_cursor = FALSE;

static gboolean
logview_show_busy_cursor (LogviewWindow *logview)
{
	GdkCursor *cursor;
	GObject *model;
	g_assert (logview->curlog);
	g_object_get (G_OBJECT (logview->curlog), "model", &model, NULL);
	if (GTK_WIDGET_VISIBLE (logview->view) && model == NULL) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (GTK_WIDGET (logview)->window, cursor);
		gdk_cursor_unref (cursor);
		gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
		busy_cursor = TRUE;
	}
	return (FALSE);
}  

static void
logview_show_normal_cursor (LogviewWindow *logview)
{
	if (busy_cursor) {
		gdk_window_set_cursor (GTK_WIDGET (logview)->window, NULL);    
		gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (logview)));
		busy_cursor = FALSE;
	}
}

void
row_toggled_cb (GtkTreeView *treeview, GtkTreeIter *iter,
                GtkTreePath *path, gpointer user_data)
{
	GtkTreeModel *model;
	Day *day;

	model = gtk_tree_view_get_model (treeview);
	gtk_tree_model_get (model, iter, DAY_POINTER, &day, -1);
	day->expand = gtk_tree_view_row_expanded (treeview, day->path);
}

static int
tree_path_find_row (GtkTreeModel *model, GtkTreePath *path, gboolean has_date)
{
	int row = 0;
	int j;
	int *indices;
	GtkTreeIter iter;

	g_assert (GTK_IS_TREE_MODEL (model));
	g_assert (path);
    
	indices = gtk_tree_path_get_indices (path);
    
	if (has_date) {
		for (j = 0; j < indices[0]; j++) {
			GtkTreePath *date_path;
			date_path = gtk_tree_path_new_from_indices (j, -1);
			gtk_tree_model_get_iter (model, &iter, date_path);
			gtk_tree_path_free (date_path);
			row += gtk_tree_model_iter_n_children (model, &iter);
		}
		if (gtk_tree_path_get_depth (path) > 1)
			row += indices[1];
        
	} else
		row = indices[0];

	return (row);
}

void
selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
	LogviewWindow *logview = data;
	GtkTreePath *selected_path;
	GList *selected_paths;
	GtkTreeModel *model;
	gboolean groupable;
	Log *log;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));

	log = logview->curlog;
	if (log == NULL)
		return;
	g_object_get (G_OBJECT (log),
		      "groupable", &groupable,
		      NULL);
	selected_paths = gtk_tree_selection_get_selected_rows (selection, &model);

	if (selected_paths == NULL)
		return;    
	g_object_set (G_OBJECT (log), "selected-paths", selected_paths, NULL);
	selected_path = gtk_tree_path_copy (g_list_first (selected_paths)->data);

	if (groupable) {
		GtkTreeIter iter;
		Day *day;
      
		if (gtk_tree_path_get_depth (selected_path) > 1)
			gtk_tree_path_up (selected_path);
		gtk_tree_model_get_iter (model, &iter, selected_path);
		gtk_tree_model_get (model, &iter, DAY_POINTER, &day, -1);
		calendar_select_date (CALENDAR (logview->calendar), day->date);
	}
	gtk_tree_path_free (selected_path);
}

static void
logview_update_statusbar (LogviewWindow *logview)
{
	char *statusbar_text;
	char *size, *modified, *index;
	time_t time;
	guint64 log_size;
	guint total_lines;
	Log *log;

	g_assert (LOGVIEW_IS_WINDOW (logview));

	log = logview->curlog;

	if (log == NULL) {
		gtk_statusbar_pop (GTK_STATUSBAR (logview->statusbar), 0);
		return;
	}
   
	/* ctime returned string has "\n\0" causes statusbar display a invalid char */
	g_object_get (G_OBJECT (log),
		      "size", &log_size,
		      "mtime", (glong *)&time,
		      "total-lines", &total_lines,
		      NULL);
	modified = ctime (&time);
	index = strrchr (modified, '\n');
	if (index && *index != '\0')
		*index = '\0';

	modified = g_strdup_printf (_("last update: %s"), modified);
	size = gnome_vfs_format_file_size_for_display (log_size);
	statusbar_text = g_strdup_printf (_("%d lines (%s) - %s"),
					  total_lines, size, modified);
	if (statusbar_text) {
		gtk_statusbar_pop (GTK_STATUSBAR(logview->statusbar), 0);
		gtk_statusbar_push (GTK_STATUSBAR(logview->statusbar), 0, statusbar_text);
		g_free (size);
		g_free (modified);
		g_free (statusbar_text);
	}
}

void
logview_update_version_bar (LogviewWindow *logview)
{
	gint i, selected;
	gchar *label;
	Log *log;
	Log **older_logs;
	gint current_version;
	gint versions;

	/* recreate the list, and emit the set active item signal */
	if (logview->curlog == NULL) {
		gtk_widget_hide_all (logview->version_bar);
		return;
	}
	g_assert (LOGVIEW_IS_LOG (logview->curlog));
	/* get parent log. if only it hasn't parent log, update achive list. */
	g_object_get (G_OBJECT (logview->curlog),
		      "parent", &log,
		      NULL);
	/* log is always the parent */
	if (log == NULL) {
		log = logview->curlog;
		/* retrieve children list */
		g_object_get (G_OBJECT (log),
			      "archives", &older_logs,
			      "versions", &versions,
			      NULL);
		gtk_widget_hide_all (logview->version_bar);
		
		/* the maximum text items of the combo box togather with "Current"
		   is OLD_LOG_NUM */
		for (i = OLD_LOG_NUM; i >= 0; i--)
			gtk_combo_box_remove_text (
				GTK_COMBO_BOX (logview->version_selector),
				i);

		gtk_combo_box_append_text (
			GTK_COMBO_BOX (logview->version_selector),
			_("Current"));

		if (versions > 0) {
			for (i=0; i<OLD_LOG_NUM; i++) {
				gchar *n;
				if (older_logs[i] == NULL)
					continue;
				g_object_get (G_OBJECT (older_logs[i]),
					      "path", &n,
					      NULL);
				g_assert (n);
				g_free (n);
				label = g_strdup_printf ("Archive %d", i);
				gtk_combo_box_append_text (
					GTK_COMBO_BOX (logview->version_selector),
					label);
				g_free (label);
			}
			gtk_widget_show_all (logview->version_bar);
		}
	}
	g_object_get (G_OBJECT (log),
		      "current-version", &current_version, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (logview->version_selector),
				  current_version);
}

static void
logview_scroll_and_focus_path (LogviewWindow *logview, Log *log)
{
	LV_MARK;
	GtkTreePath *last_bold_row;
	GtkTreePath *visible_first;
	GList *selected_paths, *idx;
	GtkTreePath *selected_path;

	g_assert (LOGVIEW_IS_WINDOW (logview));
    
	if (log == NULL)
		return;

	g_object_get (G_OBJECT (log),
		      "last-bold-row", &last_bold_row,
		      "visible-first", &visible_first,
		      "selected-paths", &selected_paths,
		      NULL);

	if (selected_paths) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (logview->view));
		for (idx = selected_paths; idx; idx = g_list_next (idx)) {
			GtkTreePath *selected_path = idx->data;
			gtk_tree_selection_select_path (selection, selected_path);
		}
	}

	if (last_bold_row != NULL) {
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (logview->view),
					      last_bold_row,
					      NULL, TRUE, 1.0, 1);   
	} else {
		if (visible_first != NULL) {
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (logview->view),
						      visible_first,
						      NULL, TRUE, 0.0, 1);   
		} 
	}    
}

static void
logview_set_log_model (LogviewWindow *window, Log *log)
{    
	GSList *days, *idx;
	Day *day;
	GtkTreeModel *model;
	GtkTreeModelFilter *filter;
	gboolean groupable;

	g_assert (LOGVIEW_IS_WINDOW (window));
	g_assert (log);

	logview_show_busy_cursor (window);
	g_object_get (G_OBJECT (log),
		      "filter", &filter,
		      "model", &model,
		      "groupable", &groupable,
		      "days", &days,
		      NULL);

	g_assert (GTK_IS_TREE_MODEL (model));

	if (filter != NULL)
		gtk_tree_view_set_model (GTK_TREE_VIEW (window->view),
					 GTK_TREE_MODEL (filter));
	else
		gtk_tree_view_set_model (GTK_TREE_VIEW (window->view),
					 model);

	if (groupable) {
		for (idx = days; idx != NULL; idx = g_slist_next(idx)) {
			day = idx->data;
			if (day->expand)
				gtk_tree_view_expand_row (GTK_TREE_VIEW (window->view),
							  day->path, FALSE);
		}
	}
	logview_show_normal_cursor (window);
}

void
logview_repaint (LogviewWindow *logview)
{
	/* if log has updated, get new lines and repaint the view */
	Log *log;
	GtkTreeModel *model;
	gboolean monitoring;

	g_return_if_fail (LOGVIEW_IS_WINDOW (logview));
    
	log = logview->curlog;
	if (log == NULL) {
		gtk_tree_view_set_model (GTK_TREE_VIEW (logview->view), NULL);
		return;
	}

	g_assert (LOGVIEW_IS_LOG (log));
	g_object_get (G_OBJECT (log),
		      "model", &model,
		      "monitoring", &monitoring,
		      NULL);
	g_assert (model);
	
	logview_set_window_title (logview);

	if (monitoring && log_has_been_modified (log)) {
		log_notify (log, TH_EVENT_UPDATE);
	}

	logview_update_statusbar (logview);

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (logview->view)) != model)
		logview_set_log_model (logview, log);

	logview_scroll_and_focus_path (logview, log);
}
