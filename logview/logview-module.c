/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

 
#include "logview-module.h"
#include "logview-debug.h"

/* default routines for implement the plugin interfaces */
static gboolean logview_module_load (LogviewModule* self);
static void logview_module_unload (LogviewModule* self);
static void logview_module_class_init (gpointer g_class, gpointer g_class_data);
static void logview_module_instance_init (GTypeInstance *instance, gpointer g_class);

static GObjectClass *parent_class = NULL;

struct _LogviewModulePrivate {
	GModule *handle;
	GType type;
	LogviewPluginInfo *info;
};

static void
module_finalize (LogviewModule* self)
{
	g_free (self->prv);
	parent_class->finalize (G_OBJECT (self));
}

static void
logview_module_class_init (gpointer g_class, gpointer g_class_data)
{
	GObjectClass *object_class;
	GTypeModuleClass *self;

	object_class = G_OBJECT_CLASS (g_class);
	self = G_TYPE_MODULE_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);
	object_class->finalize = (void (*)(GObject*)) module_finalize;

	self->load = (gboolean (*)(GTypeModule*)) logview_module_load;
	self->unload = (void (*)(GTypeModule*)) logview_module_unload;
}

static void
logview_module_instance_init (GTypeInstance *instance, gpointer g_class)
{
	LogviewModule *self = LOGVIEW_MODULE (instance);
	self->prv = g_new0 (LogviewModulePrivate, 1);
}

GType
logview_module_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewModuleClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			logview_module_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LogviewModule),
			0,      /* n_preallocs */
			logview_module_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_TYPE_MODULE, "LogviewModuleType", &info, 0);
	}
	return type;
}

LogviewModule*
logview_new_module (const gchar* module_path)
{
	LogviewModule* new_module = NULL;

	LV_MARK;
	g_assert (module_path != NULL);
	new_module = g_object_new (LOGVIEW_TYPE_MODULE, NULL);
	g_type_module_set_name (G_TYPE_MODULE (new_module), module_path);
	return new_module;
}

LogviewPlugin*
logview_module_new_plugin (const LogviewModule* self)
{
	LogviewPlugin *plugin;
	LV_MARK;
	g_assert (self->prv->type != 0);
	plugin = g_object_new (self->prv->type, NULL);
	g_assert (LOGVIEW_IS_PLUGIN(plugin));
	return LOGVIEW_PLUGIN (plugin);
}

static gboolean
logview_module_load (LogviewModule* self)
{
	GType (*register_logview_plugin)(GTypeModule*);
	LogviewPluginInfo *(*logview_plugin_info)();

	LV_MARK;

	self->prv->handle = g_module_open (logview_module_name (self), G_MODULE_BIND_LOCAL);
	if (self->prv->handle !=NULL) {
		gboolean hit;
		hit = g_module_symbol (self->prv->handle, "register_logview_plugin", (gpointer*)(&register_logview_plugin));
		g_return_val_if_fail (hit != FALSE, FALSE);
		hit = g_module_symbol (self->prv->handle, "logview_plugin_info", (gpointer*)(&logview_plugin_info));
		g_return_val_if_fail (hit != FALSE, FALSE);

		self->prv->type = (*register_logview_plugin) ((GTypeModule*) self);
		self->prv->info = (*logview_plugin_info) ();
		
		g_return_val_if_fail (self->prv->type != 0, FALSE);
		g_return_val_if_fail (self->prv->info != NULL, FALSE);
		return TRUE;
	}
	else {
		LV_ERR ("logview_module_load error: %s", g_module_error());
	}
	return FALSE;
}

static void
logview_module_unload (LogviewModule* self)
{
	LV_MARK;
	g_module_close (self->prv->handle);
	self->prv->handle = NULL;
	self->prv->type = 0;
	self->prv->info = NULL;
}

const gchar*
logview_module_name (LogviewModule *self)
{
	g_assert (G_IS_TYPE_MODULE (self));
	return G_TYPE_MODULE (self)->name;
}

GType
logview_module_type (LogviewModule *self)
{
	return self->prv->type;
}

PluginPrio
logview_module_priority (LogviewModule *self)
{
	return self->prv->info->prio;
}

const gchar*
logview_module_desc (LogviewModule *self)
{
	return self->prv->info->desc;
}
