/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

 
#ifndef __LOGVIEW_PLUGINMGR_H__
#define __LOGVIEW_PLUGINMGR_H__
#include <glib-object.h>
#include <glib.h>
#include "logview-module.h"
#include "logrtns.h"

typedef struct _LogviewPluginMgr LogviewPluginMgr;
typedef struct _LogviewPluginMgrClass LogviewPluginMgrClass;
typedef struct _LogviewPluginMgrPrivate LogviewPluginMgrPrivate;

typedef void (*ModuleHandleCB) (const LogviewModule *module, gpointer data);

struct _LogviewPluginMgr {
	GObject parent;
	/* instance members */
/* private */
	LogviewPluginMgrPrivate *prv;
};

struct _LogviewPluginMgrClass {
	GObjectClass parent;

	/* class members */

	/* handle one module */
	gboolean (*load_module) (LogviewPluginMgr *self, const gchar *module_path);
	void (*unload_module) (LogviewPluginMgr *self, LogviewModule *module);

	/* handle all module in paths */
	gboolean (*load_modules)(LogviewPluginMgr *self);
	void (*unload_modules)(LogviewPluginMgr *self);

	/* collect all the path names in Plugin paths.
	   return a list, do not try to free it.
	 */
	GSList* (*compose_search_plugin_paths)(LogviewPluginMgr *self);
	
	void (*iterate)(LogviewPluginMgr *self, ModuleHandleCB cb, gpointer data);
};

/* For the core application which should implemete the level to call plugins */

//void logview_plugins_detect (const gchar* plugin_patchs);


extern void pluginmgr_init ();
extern void pluginmgr_destroy ();
extern gboolean pluginmgr_load_modules ();
extern void pluginmgr_unload_modules ();
extern gint pluginmgr_plugin_amount();

extern GSList* pluginmgr_get_all_logs ();
extern Log* pluginmgr_new_log_from_path (const gchar* log_path);
extern void pluginmgr_modules_iterate (ModuleHandleCB cb, gpointer data);

/* routines provided for internal usage */

#endif /* __LOGVIEW_PLUGINMGR_H__ */

