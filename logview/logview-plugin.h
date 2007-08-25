/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_PLUGIN_H__
#define __LOGVIEW_PLUGIN_H__

#include <glib-object.h>
#include <gmodule.h>
#include "logview-iface.h"

G_BEGIN_DECLS

#define LOGVIEW_TYPE_PLUGIN		(logview_plugin_get_type ())
#define LOGVIEW_PLUGIN(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), LOGVIEW_TYPE_PLUGIN, LogviewPlugin))
#define LOGVIEW_PLUGIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), LOGVIEW_TYPE_PLUGIN, LogviewPluginClass))
#define LOGVIEW_IS_PLUGIN(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), LOGVIEW_TYPE_PLUGIN))
#define LOGVIEW_IS_PLUGIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), LOGVIEW_TYPE_PLUGIN))
#define LOGVIEW_PLUGIN_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), LOGVIEW_TYPE_PLUGIN, LogviewPluginClass))

typedef struct _LogviewPluginInfo LogviewPluginInfo;
typedef struct _LogviewPlugin LogviewPlugin;
typedef struct _LogviewPluginClass LogviewPluginClass;

/**
 * PluginPrio:
 * @PLUG_PRIO_BASE: the base priority. if a plugin is explicited defined
 * a priority by redefine #PLUGIN_PRIO, it will be assigned to %PLUG_PRIO_USER.
 * @PLUG_PRIO_USER: the base line of priority which user should use,
 * e.g. %PLUG_PRIO_USER + 1
 * 
 * A plugin with a high priority will be used first.
 */
typedef enum {
	PLUG_PRIO_BASE=0,
	PLUG_PRIO_USER=10
} PluginPrio;

/*< private >*/
typedef enum {
	PF_LOG_IO=0,
	PF_LOG_VIEW,
	PF_LOG_COLLECTOR,
	PF_LOG_NUM
} PluginFunc;

/*< private >*/
#define LOG_PF_NUM PF_LOG_COLLECTOR
/*< private >*/
#define MGR_PF_NUM PF_LOG_NUM

/*< private >*/
struct _LogviewPluginInfo {
	PluginPrio prio;
	gchar *desc;
};

/*< public >*/
struct _LogviewPlugin {
	/*< private >*/
	GObject parent;
};

struct _LogviewPluginClass {
	GObjectClass parent;
	
/* virtual public */
	gboolean (*plugin_init) (LogviewPlugin* self);
	void (*plugin_destroy) (LogviewPlugin* self);
};

GType logview_plugin_get_type (void);
gboolean logview_plugin_init (LogviewPlugin* self);
void logview_plugin_destroy (LogviewPlugin* self);

/**
 * PLUGIN_PRIO:
 * 
 * Define plugin priority, can be redefined by users.
 * 
 * PLUGIN_PRIO should be defined great or equal to PLUG_PRIO_USER,
 * so that your plugin can be probed first.
 * 
 * e.g. %PLUG_PRIO_USER + 1
 */
#define PLUGIN_PRIO	PLUG_PRIO_USER

/**
 * PLUGIN_DESC:
 * 
 * A brief description for the plugin is needed
 * for displaying the plugin information in GUI.
 * 
 * e.g. PLUGIN_DESC	"For ..."
 */
#define PLUGIN_DESC	"Unknown plugin."

/**
 * LOGVIEW_INIT_PLUGIN:
 * @PluginTypeFunc: a function reports the #GType of the plugin.
 * 
 * A macro exports some useful functions. Therefor core application
 * can get some useful information to load the plugin.
 */
#define LOGVIEW_INIT_PLUGIN(PluginTypeFunc)	\
G_MODULE_EXPORT LogviewPluginInfo* logview_plugin_info (void);		\
G_MODULE_EXPORT LogviewPluginInfo* logview_plugin_info (void)		\
{									\
	static LogviewPluginInfo __plugin_info;				\
	__plugin_info.prio = PLUGIN_PRIO;				\
	__plugin_info.desc = PLUGIN_DESC;				\
	return &__plugin_info;						\
}									\
G_MODULE_EXPORT GType register_logview_plugin (GTypeModule* module);	\
G_MODULE_EXPORT GType register_logview_plugin (GTypeModule* module)	\
{									\
	return PluginTypeFunc (module);					\
}


G_END_DECLS

#endif /* __LOGVIEW_PLUGIN_H__ */

