/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include "logview-plugin-list.h"
#include "logview-plugin-manager.h"
#include "logview-module.h"
#include "misc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static void logview_plugin_list_init (LogviewPluginList *plugin_list);

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

enum {
	NAME_COL,
	PRI_COL,
	DESC_COL,
	PATH_COL,
	N_COLS
};

static void
logview_plugin_list_sync_cb (const LogviewModule *module, gpointer data)
{
	GtkTreeIter iter;
	gchar *name, *desc, *path;
	gint pri;
	LogviewPluginList *self = (LogviewPluginList*)data;

	path = logview_module_name (module);
	pri = logview_module_priority (module);
	desc = logview_module_desc (module);
	name = log_extract_filename (path);

	gtk_tree_store_append (self->list, &iter, NULL);
	gtk_tree_store_set (self->list, &iter,
			    NAME_COL, name,
			    PRI_COL, pri,
			    DESC_COL, desc,
			    PATH_COL, path,
			    -1);
	g_free (name);
}

static void
logview_plugin_list_sync (LogviewPluginList *self)
{
	gtk_widget_hide_all (self->scrolledwindow);
	gtk_widget_hide_all (self->label);
	gtk_tree_store_clear (self->list);
	if (pluginmgr_plugin_amount () > 0) {
		pluginmgr_modules_iterate (logview_plugin_list_sync_cb, self);
		gtk_widget_show_all (self->scrolledwindow);
	}
	else
		gtk_widget_show_all (self->label);

}

void
logview_plugin_list_show (LogviewPluginList *self, GtkWidget *window)
{
	g_return_if_fail (GTK_IS_WINDOW (window));
	gtk_widget_show_all (self->dialog);
	logview_plugin_list_sync (self);
}

static gint
logview_plugin_list_cmp (GtkTreeModel *model,
			    GtkTreeIter *a,
			    GtkTreeIter *b,
			    gpointer user_data)
{
	gint col_id = GPOINTER_TO_INT(user_data);
	gint ret;

	switch (col_id) {
	case PATH_COL:
	case NAME_COL: {
		gchar *stra, *strb;
		gtk_tree_model_get (model, a, (gint)user_data, &stra, -1);
		gtk_tree_model_get (model, b, (gint)user_data, &strb, -1);
		ret = g_ascii_strcasecmp (stra, strb);
		g_free (stra);
		g_free (strb);
	}
		break;
	case PRI_COL: {
		gint ia, ib;
		gtk_tree_model_get (model, a, (gint)user_data, &ia, -1);
		gtk_tree_model_get (model, b, (gint)user_data, &ib, -1);
		ret = ia - ib;
	}
		break;
	default:
		g_assert_not_reached ();
	}
	return ret;
}

static void
logview_plugin_list_init (LogviewPluginList *self)
{
	GtkWidget *dialog_vbox3;
	GtkWidget *dialog_action_area3;
	GtkWidget *exist_button;
	GtkTreeViewColumn *name_col, *pri_col, *path_col, *desc_col;
	GtkCellRenderer *text_renderer;
	GtkWidget *view;

	self->list = gtk_tree_store_new (N_COLS,
					 G_TYPE_STRING,
					 G_TYPE_INT,
					 G_TYPE_STRING,
					 G_TYPE_STRING);
	text_renderer = gtk_cell_renderer_text_new ();

	view = g_object_new (GTK_TYPE_TREE_VIEW,
			     "model", self->list,
			     "rules-hint", TRUE,
			     "headers-clickable", TRUE,
			     "reorderable", TRUE,
			     NULL);

	name_col = gtk_tree_view_column_new_with_attributes (_("Plugin Name"),
							     text_renderer,
							     "text", NAME_COL,
							     NULL);
	gtk_tree_view_column_set_sort_column_id (name_col, NAME_COL);

	g_object_set (name_col,
		      "resizable", TRUE,
		      "clickable", TRUE,
		      "sort-indicator", TRUE,
		      "reorderable", TRUE,
		      NULL);
	pri_col = gtk_tree_view_column_new_with_attributes (_("Priority"),
							     text_renderer,
							     "text", PRI_COL,
							     NULL);
	gtk_tree_view_column_set_sort_column_id (pri_col, PRI_COL);

	g_object_set (pri_col,
		      "resizable", TRUE,
		      "clickable", TRUE,
		      "sort-indicator", TRUE,
		      "reorderable", TRUE,
		      NULL);
	desc_col = gtk_tree_view_column_new_with_attributes (_("Description"),
							     text_renderer,
							     "text", DESC_COL,
							     NULL);
	g_object_set (desc_col,
		      "resizable", TRUE,
		      "reorderable", TRUE,
		      NULL);

	path_col = gtk_tree_view_column_new_with_attributes (_("Path"),
							     text_renderer,
							     "text", PATH_COL,
							     NULL);
	g_object_set (path_col,
		      "resizable", TRUE,
		      "clickable", TRUE,
		      "sort-indicator", TRUE,
		      "reorderable", TRUE,
		      NULL);
	gtk_tree_view_column_set_sort_column_id (path_col, PATH_COL);

	gtk_tree_sortable_set_sort_func	(GTK_TREE_SORTABLE(self->list),
					 NAME_COL,
					 (GtkTreeIterCompareFunc) logview_plugin_list_cmp,
					 GINT_TO_POINTER(NAME_COL),
					 NULL);
	gtk_tree_sortable_set_sort_func	(GTK_TREE_SORTABLE(self->list),
					 PATH_COL,
					 (GtkTreeIterCompareFunc) logview_plugin_list_cmp,
					 GINT_TO_POINTER(PATH_COL),
					 NULL);
	gtk_tree_sortable_set_sort_func	(GTK_TREE_SORTABLE(self->list),
					 PRI_COL,
					 (GtkTreeIterCompareFunc) logview_plugin_list_cmp,
					 GINT_TO_POINTER(PRI_COL),
					 NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(self->list), NAME_COL, GTK_SORT_ASCENDING);

	gtk_tree_view_append_column (view, name_col);
	gtk_tree_view_append_column (view, pri_col);
	gtk_tree_view_append_column (view, desc_col);
	gtk_tree_view_append_column (view, path_col);

	self->dialog = gtk_dialog_new ();
	gtk_dialog_set_has_separator (GTK_DIALOG (self->dialog), FALSE);
	gtk_widget_set_size_request (self->dialog, 800, 400);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (self->dialog), TRUE);
	gtk_window_set_title (GTK_WINDOW (self->dialog), _("Log Viewer Plugins"));
	gtk_window_set_position (GTK_WINDOW (self->dialog),
				 GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal (GTK_WINDOW (self->dialog), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (self->dialog), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (self->dialog),
				  GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (self->dialog), TRUE);

	dialog_vbox3 = GTK_DIALOG (self->dialog)->vbox;

	self->label = gtk_label_new (_("No plugins installed!"));
	gtk_widget_show (self->label);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), self->label, TRUE, TRUE, 0);

	self->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), self->scrolledwindow,
			    TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->scrolledwindow),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->scrolledwindow),
					     GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (self->scrolledwindow), view);

	dialog_action_area3 = GTK_DIALOG (self->dialog)->action_area;
	gtk_container_set_border_width (GTK_CONTAINER (self->scrolledwindow), 12);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3),
				   GTK_BUTTONBOX_END);

	exist_button = gtk_button_new_from_stock ("gtk-close");
	gtk_dialog_add_action_widget (GTK_DIALOG (self->dialog),
				      exist_button, GTK_RESPONSE_CLOSE);
	GTK_WIDGET_SET_FLAGS (exist_button, GTK_CAN_DEFAULT);

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF (self->dialog, dialog_vbox3, "dialog_vbox3");
	GLADE_HOOKUP_OBJECT (self->dialog, self->scrolledwindow, "scrolledwindow");
	GLADE_HOOKUP_OBJECT (self->dialog, view, "treeview1");
	GLADE_HOOKUP_OBJECT_NO_REF (self->dialog, dialog_action_area3, "dialog_action_area3");
	GLADE_HOOKUP_OBJECT (self->dialog, exist_button, "exist_button");
	GLADE_HOOKUP_OBJECT (self->dialog, self->label, "label1");

	g_signal_connect_swapped (exist_button, "clicked",
				  G_CALLBACK(gtk_widget_hide_all),
				  self->dialog);
	g_signal_connect_swapped (self->dialog, "delete-event",
				  G_CALLBACK(gtk_widget_hide_on_delete),
				  self->dialog);

}

LogviewPluginList *
logview_plugin_list_new (void)
{
	LogviewPluginList * list;
	list = g_new0 (LogviewPluginList, 1);
	logview_plugin_list_init (list);
	return list;
}

void
logview_plugin_list_delete (LogviewPluginList * list)
{
	g_free (list);
}
