/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_MODULE_H__
#define __LOGVIEW_MODULE_H__

#include <glib-object.h>
#include <gmodule.h>
#include "logview-plugin.h"

#define LOGVIEW_TYPE_MODULE		(logview_module_get_type ())
#define LOGVIEW_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_MODULE, LogviewModule))
#define LOGVIEW_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_MODULE, LogviewModuleClass))
#define LOGVIEW_IS_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_MODULE))
#define LOGVIEW_IS_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_MODULE))
#define LOGVIEW_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_MODULE, LogviewModuleClass))

typedef struct _LogviewModule LogviewModule;
typedef struct _LogviewModuleClass LogviewModuleClass;
typedef struct _LogviewModulePrivate LogviewModulePrivate;

struct _LogviewModule{
	GTypeModule parent;
	/* instance members */
/* private */
	LogviewModulePrivate *prv;
};

struct _LogviewModuleClass {
	GTypeModuleClass parent;
	/* class members */
};


/* For the core application which should implemete the level to call plugins */
GType logview_module_get_type (void);
/* return the name of the module, should not be freed */
const gchar* logview_module_name (LogviewModule *self);
GType logview_module_type (LogviewModule *self);
PluginPrio logview_module_priority (LogviewModule *self);
const gchar* logview_module_desc (LogviewModule *self);
/* return a new module */
LogviewModule* logview_new_module (const gchar* module_path);
/* return a new plugin */
LogviewPlugin* logview_module_new_plugin (const LogviewModule* self);

#endif /* __LOGVIEW_MODULE_H__ */

