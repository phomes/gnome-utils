/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#include "logview-iface.h"
#include "logview-iface-collector.h"
#include "logview-debug.h"

static void
logview_iface_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}

static void
logview_iface_class_init (gpointer g_class, gpointer g_class_data)
{
	LogviewIFaceCollector *self = (LogviewIFaceCollector *)g_class;
	self->config_log = NULL;
	self->config_log_from_path = NULL;
	self->config_log = NULL;
}

GType
logview_iface_collector_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewIFaceCollector),
			logview_iface_base_init,   /* base_init */
			NULL,   /* base_finalize */
			logview_iface_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE,
					       g_intern_static_string ("LogviewIFaceCollectorType"),
					       &info, 0);
		g_type_interface_add_prerequisite (type, LOGVIEW_TYPE_IFACE);
	}
	return type;
}

/**
 * logi_get_logs:
 * @self: a #LogviewICollector instance.
 * 
 * Gnome System Log get logs by invoking this routine.
 * 
 * Returns: a single list, each element is a string represents a log name.
 **/
GSList*
logi_get_logs (LogviewICollector *self)
{
	LV_MARK;
	return LOGVIEW_GET_COLLECTOR_INTERFACE (self)->get_logs(self);
}

/**
 * logi_config_log_from_path:
 * @self: a #LogviewICollector instance.
 * @log_name: log name which is obtained from logi_get_logs().
 * 
 * Gnome System Log tries to config a log instance by passing the log name to this routine.
 *
 * Returns: On successful completion, returns a #Log instance; else returns NULL.
 **/
Log* 
logi_config_log_from_path (LogviewICollector *self, const gchar *log_name)
{
	LV_MARK;
	return LOGVIEW_GET_COLLECTOR_INTERFACE (self)->config_log_from_path (self, log_name);
}

/**
 * logi_config_log:
 * @self: a #LogviewICollector instance.
 * @log: a #Log instance.
 * 
 * Gnome System Log tries to config a log instance by passing the @log to this routine.
 *
 * Returns: On successful completion, returns TRUE; else returns FALSE.
 **/
gboolean
logi_config_log (LogviewICollector *self, Log *log)
{
	LV_MARK;
	return LOGVIEW_GET_COLLECTOR_INTERFACE (self)->config_log (self, log);
}

