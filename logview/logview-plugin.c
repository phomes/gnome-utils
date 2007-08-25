/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

 
#include "logview-plugin.h"
#include "logview-debug.h"

static GObjectClass *parent_class = NULL;
static void logview_plugin_finalize (GObject *object);

static void
logview_plugin_instance_init (LogviewPlugin *self, gpointer g_class_data)
{
	LV_MARK;
}

static void
logview_plugin_class_init (gpointer g_class, gpointer g_class_data)
{
	GObjectClass *object_class;
	LogviewPluginClass *self;
	
	LV_MARK;

	object_class = G_OBJECT_CLASS (g_class);
	self = LOGVIEW_PLUGIN_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);
	self->plugin_init = NULL;
	self->plugin_destroy = NULL;
	object_class->finalize = logview_plugin_finalize;
}

GType
logview_plugin_get_type (void)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewPluginClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			logview_plugin_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LogviewPlugin),
			0,      /* n_preallocs */
			(GInstanceInitFunc) logview_plugin_instance_init    /* instance_init */
		};		
		type = g_type_register_static (G_TYPE_OBJECT,
					       "LogviewPluginType",
					       &info, 0);
	}
	return type;
}

static void
logview_plugin_finalize (GObject *object)
{
	LV_MARK;
	if (LOGVIEW_PLUGIN_GET_CLASS (object)->plugin_destroy)
		LOGVIEW_PLUGIN_GET_CLASS (object)->plugin_destroy (LOGVIEW_PLUGIN (object));
	parent_class->finalize (object);
}

/**
 * logview_plugin_init:
 * @self: a #LogviewPlugin instance.
 * 
 * plugin_init default returns TRUE if it is not overriden.
 * If it returns FALSE, the plugin will be skipped.
 * 
 * This method will be called by manager after interface can_handle
 * is called. The plugin should be initialized at this time. 
 * If returns TRUE, this plugin will be assigned to log.
 * 
 * Returns: Upon a successful completion or it is not be overloaded, returns TRUE.
 */
gboolean
logview_plugin_init (LogviewPlugin* self)
{
	LV_MARK;
	if (LOGVIEW_PLUGIN_GET_CLASS(self)->plugin_init)
		return LOGVIEW_PLUGIN_GET_CLASS(self)->plugin_init (self);
	return TRUE;
}

/**
 * logview_plugin_destroy:
 * @self: a #LogviewPlugin instance.
 * 
 * Log instance will automatically destroy all plugins it has.
 * This method will be automatically called when it's unrefed.
 */
void
logview_plugin_destroy (LogviewPlugin* self)
{
	LV_MARK;
	if (LOGVIEW_PLUGIN_GET_CLASS(self)->plugin_destroy)
		LOGVIEW_PLUGIN_GET_CLASS(self)->plugin_destroy (self);
}

