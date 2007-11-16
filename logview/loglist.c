/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include "logview.h"
#include "misc.h"
#include "loglist.h"
#include "logrtns.h"
#include <libgnomevfs/gnome-vfs.h>
#include <logview-debug.h>

#define BOLD_LOG_TIME	5000

struct LogListPrv {
	GtkTreeStore *model;
	gpointer window;
	guint unbold_event;
};

enum {
	LOG_NAME = 0,
	LOG_POINTER,
	LOG_WEIGHT,
	LOG_WEIGHT_SET
};

enum {
	PROP_0 = 0,
	PROP_WINDOW
};

static void loglist_set_property (GObject      *object,
				  guint         property_id,
				  const GValue *value,
				  GParamSpec   *pspec);

G_DEFINE_TYPE (LogList, loglist, GTK_TYPE_TREE_VIEW)

static GtkTreePath *
loglist_find_directory (LogList *list, gchar *dir)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	gchar *iterdir;
	
	g_assert (LOG_IS_LIST (list));

	model = GTK_TREE_MODEL (list->prv->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_get (model, &iter, LOG_NAME, &iterdir, -1);
		if (iterdir) {
			gint val;
			val = g_ascii_strncasecmp (iterdir, dir, -1);
			g_free (iterdir);
			if (val == 0) {
				path = gtk_tree_model_get_path (model, &iter);
				break;
			}
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	return path;
}


/* The returned GtkTreePath needs to be freed */

static GtkTreePath *
loglist_find_log (LogList *list, Log *log)
{
	GtkTreeIter iter, child_iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	Log *iterlog;
	
	g_assert (LOG_IS_LIST (list));
	g_assert (log != NULL);
	LV_MARK;

	model = GTK_TREE_MODEL (list->prv->model);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;
	
	do {
		gtk_tree_model_iter_children (model, &child_iter, &iter);
		do {
			gtk_tree_model_get (model, &child_iter, LOG_POINTER, &iterlog, -1);
			if (iterlog == log) {
				path = gtk_tree_model_get_path (model, &child_iter);
				return path;
			}
		} while (gtk_tree_model_iter_next (model, &child_iter));
	} while (gtk_tree_model_iter_next (model, &iter));

	return NULL;
}

static void
loglist_get_log_iter (LogList *list, Log *log, GtkTreeIter *logiter)
{
    GtkTreeModel *model;
    GtkTreePath *path = NULL;
    LV_MARK;

    path = loglist_find_log (list, log);
    if (path) {
	    model = GTK_TREE_MODEL (list->prv->model);
	    gtk_tree_model_get_iter (model, logiter, path);
	    gtk_tree_path_free (path);
    }
}

static gboolean
loglist_unbold (LogList *self)
{
	LogviewWindow *logview = LOGVIEW_WINDOW(self->prv->window);
	GtkTreeModel *model;
	GtkTreeIter iter;
	LV_MARK;

	model = GTK_TREE_MODEL (self->prv->model);

	loglist_get_log_iter (self, logview_get_active_log (logview),
			      &iter);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			    LOG_WEIGHT_SET, FALSE, -1);

	self->prv->unbold_event = 0;
	g_object_unref (self);
	return FALSE;
}

void
loglist_bold_log (LogList *list, Log *log)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    LogviewWindow *logview = LOGVIEW_WINDOW(list->prv->window);
    LV_MARK;
    
    g_return_if_fail (LOG_IS_LIST (list));
    g_return_if_fail (log != NULL);

    model = GTK_TREE_MODEL (list->prv->model);

    loglist_get_log_iter (list, log, &iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
			LOG_WEIGHT, PANGO_WEIGHT_BOLD,
			LOG_WEIGHT_SET, TRUE, -1);

    /* if the log is currently shown, put up a timer to unbold it */
    if (logview_get_active_log (logview) == log)
	    g_timeout_add (BOLD_LOG_TIME,
			   (GSourceFunc) loglist_unbold,
			   g_object_ref (list));
}

void
loglist_select_log (LogList *list, Log *log)
{
	GtkTreePath *path;
       	
	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log);

	path = loglist_find_log (list, log);
	if (path) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (list), path);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
}

void
loglist_remove_log (LogList *list, Log *log)
{
	GtkTreeIter iter, parent;
	gchar *name;

	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log != NULL);

	g_object_get (G_OBJECT (log), "path", &name, NULL);
	loglist_get_log_iter (list, log, &iter);
	gtk_tree_model_iter_parent (GTK_TREE_MODEL (list->prv->model), &parent, &iter);

	if (gtk_tree_store_remove (list->prv->model, &iter)) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		gtk_tree_selection_select_iter (selection, &iter);
	} else {
		if (!gtk_tree_model_iter_has_child (GTK_TREE_MODEL (list->prv->model), &parent)) {
			if (gtk_tree_store_remove (list->prv->model, &parent)) {
				GtkTreeSelection *selection;
				selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
				gtk_tree_selection_select_iter (selection, &parent);
			}
		}
	}
	g_free (name);
}

void
loglist_add_directory (LogList *list, gchar *dirname, GtkTreeIter *iter)
{
	gtk_tree_store_append (list->prv->model, iter, NULL);
	gtk_tree_store_set (list->prv->model, iter,
			    LOG_NAME, dirname, LOG_POINTER, NULL, -1);
}

void 
loglist_add_log (LogList *list, Log *log)
{
	GtkTreeIter iter, child_iter;
	GtkTreePath *path;
	gchar *filename, *dirname;

	g_return_if_fail (LOG_IS_LIST (list));
	g_return_if_fail (log != NULL);

	log_extract_filepath (log, &dirname, &filename);
	path = loglist_find_directory (list, dirname);
	if (path) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (list->prv->model), &iter, path);
		gtk_tree_path_free (path);
	} else
		loglist_add_directory (list, dirname, &iter);

	gtk_tree_store_append (list->prv->model, &child_iter, &iter);
	gtk_tree_store_set (list->prv->model, &child_iter, 
                        LOG_NAME, filename, LOG_POINTER, log, -1);

	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (list)))
		loglist_select_log (list, log);

	g_free (dirname);
	g_free (filename);
}

static void
loglist_selection_changed (GtkTreeSelection *selection, LogList *self)
{
	LogviewWindow *logview = LOGVIEW_WINDOW(self->prv->window);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean bold;
	Log *log;

	LV_MARK;
	g_assert (GTK_IS_TREE_SELECTION (selection));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		logview_select_log (logview, NULL);
		return;
	}

	gtk_tree_model_get (model, &iter, 
			    LOG_POINTER, &log, 
			    LOG_WEIGHT_SET, &bold, -1);
	/* there is a differnt selection before,
	   check if there is a pending loglist_unbold */
	if (logview_get_active_log (logview) != log) {
		if (self->prv->unbold_event > 0) {
			gboolean ret = FALSE;
			ret = g_source_remove (self->prv->unbold_event);
			g_assert (ret);
			self->prv->unbold_event = 0;
		}
	}
	logview_select_log (logview, log);
	if (bold) {
		self->prv->unbold_event = 
			g_timeout_add (BOLD_LOG_TIME,
				       (GSourceFunc) loglist_unbold,
				       g_object_ref (self));
	}
}

static void 
loglist_init (LogList *list)
{
	GtkTreeStore *model;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkCellRenderer *cell;

	list->prv = g_new0 (LogListPrv, 1);

	model = gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_BOOLEAN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
	list->prv->model = model;
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
	g_object_unref (G_OBJECT (model));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (loglist_selection_changed), list);
    
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", LOG_NAME,
					     "weight-set", LOG_WEIGHT_SET,
					     "weight", LOG_WEIGHT,
					     NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(list->prv->model), 0, GTK_SORT_ASCENDING);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (list), -1);
}

static void
loglist_finalize (GObject *object)
{
	LogList *list = LOG_LIST (object);
	g_free (list->prv);
	(*G_OBJECT_CLASS(loglist_parent_class)->finalize) (object); 
}

static void
loglist_class_init (LogListClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	object_class->finalize = loglist_finalize;
        object_class->set_property = loglist_set_property;

	pspec = g_param_spec_pointer ("window",
				      "LogviewWindow pointer",
				      "Set LogviewWindow pointer",
				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property (object_class,
					 PROP_WINDOW,
					 pspec);
}

static void
loglist_set_property (GObject      *object,
		      guint         property_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	LogList *self = LOG_LIST (object);

	switch (property_id) {
	case PROP_WINDOW:
		self->prv->window = g_value_get_pointer (value);
		g_assert (LOGVIEW_IS_WINDOW(self->prv->window));
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
}

GtkWidget *
loglist_new (gpointer window)
{
    GtkWidget *widget;
    widget = g_object_new (LOG_LIST_TYPE, "window", window, NULL);
    return widget;
}
