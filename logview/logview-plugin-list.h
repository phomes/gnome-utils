/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_PLUGIN_LIST_H__
#define __LOGVIEW_PLUGIN_LIST_H__

#include <gtk/gtk.h>

typedef struct _LogviewPluginList LogviewPluginList;
struct _LogviewPluginList {
	GtkTreeStore *list;
	GtkWidget *scrolledwindow;
	GtkWidget *label;
	GtkWidget *dialog;
};

void logview_plugin_list_show (LogviewPluginList *self, GtkWidget *window);
LogviewPluginList *logview_plugin_list_new (void);
void logview_plugin_list_delete (LogviewPluginList *);

#endif /* __LOGVIEW_PLUGIN_LIST_H__ */
