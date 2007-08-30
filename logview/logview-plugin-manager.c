/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */
 
#include <glib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <sys/utsname.h>
#include "logview-module.h"
#include "logview-plugin-manager.h"
#include "logview-iface-io.h"
#include "logview-iface-view.h"
#include "logview-iface-collector.h"
#include "logview-debug.h"
#include "misc.h"

#define LOGVIEW_TYPE_PLUGINMGR		(logview_pluginmgr_get_type ())
#define LOGVIEW_PLUGINMGR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_PLUGINMGR, LogviewPluginMgr))
#define LOGVIEW_PLUGINMGR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_PLUGINMGR, LogviewPluginMgrClass))
#define LOGVIEW_IS_PLUGINMGR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_PLUGINMGR))
#define LOGVIEW_IS_PLUGINMGR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_PLUGINMGR))
#define LOGVIEW_PLUGINMGR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_PLUGINMGR, LogviewPluginMgrClass))

static GType logview_pluginmgr_get_type (void);
static void pluginmgr_class_init (gpointer g_class, gpointer g_class_data);
static void pluginmgr_instance_init (GTypeInstance *instance, gpointer g_class);

static gboolean load_module(LogviewPluginMgr *self, const gchar* module_path);
static void unload_module (LogviewPluginMgr *self, LogviewModule* plugin_info);
static gboolean load_modules (LogviewPluginMgr *self);
static void unload_modules (LogviewPluginMgr *self);

static gboolean register_module (LogviewPluginMgr *self, LogviewModule* plugin_info);
static gboolean is_plugin_file (const gchar* filepath);
static Log* pluginmgr_new_log (Log *log);
/*-------------------------------------------------------------------------------*/
static gint module_prio_cmpfunc (gconstpointer a, gconstpointer b, gpointer user_data);
static gint module_cmpfunc (gconstpointer a, gconstpointer b);

static GObjectClass *parent_class = NULL;
static LogviewPluginMgr *pluginmgr_instance = NULL;

struct _LogviewPluginMgrPrivate {
	GType typehash[MGR_PF_NUM];
	GSList *all_modules;
	GSList *modules[MGR_PF_NUM];
	GSList *plugin_full_paths;
};

static GType
logview_pluginmgr_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewPluginMgrClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			pluginmgr_class_init,	/* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LogviewPluginMgr),
			0,      /* n_preallocs */
			pluginmgr_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "LogviewPluginMgrType",
					       &info, 0);
	}
	return type;
}

static void
pluginmgr_instance_init (GTypeInstance *instance, gpointer g_class)
{
	LogviewPluginMgr *self = LOGVIEW_PLUGINMGR (instance);
	self->prv = g_new0 (LogviewPluginMgrPrivate,1);
	self->prv->typehash[PF_LOG_IO] = LOGVIEW_TYPE_IFACE_IO;
	self->prv->typehash[PF_LOG_VIEW] = LOGVIEW_TYPE_IFACE_VIEW;
	self->prv->typehash[PF_LOG_COLLECTOR] = LOGVIEW_TYPE_IFACE_COLLECTOR;
}

static void
pluginmgr_finalize (LogviewPluginMgr *self)
{
	GSList *idx;

	LV_MARK;
	
	LOGVIEW_PLUGINMGR_GET_CLASS (self)->unload_modules (self);

	if (self->prv->plugin_full_paths) {
		for (idx = self->prv->plugin_full_paths; idx; idx = g_slist_next(idx)) {
			gchar *path = (gchar *)idx->data;
			g_free (path);
		}
		g_slist_free (self->prv->plugin_full_paths);
	}

	g_free (self->prv);
	parent_class->finalize (G_OBJECT (self));
}

static gboolean
load_modules (LogviewPluginMgr *self)
{
	LogviewPluginMgrClass* klass;
	GSList* plugin_paths;
	GSList* idx;

	LV_MARK;
	if (!g_module_supported())
		return FALSE;
	
	klass = LOGVIEW_PLUGINMGR_GET_CLASS(self);
	plugin_paths = get_plugin_paths();

	g_return_val_if_fail (plugin_paths != NULL, FALSE);

	for (idx = plugin_paths; idx; idx = idx->next) {
		/* load plugins */
		GDir *dir;
		gchar* plugin_full_path;
		gchar* search_path = idx->data;
		
		dir = g_dir_open (search_path, 0, NULL);
		if (dir != NULL) {
			while ((plugin_full_path = (gchar *) g_dir_read_name (dir)) != NULL) {
				plugin_full_path = g_build_filename (search_path, plugin_full_path, NULL);
				if (klass->load_module (self, plugin_full_path)) {
					LV_INFO ("module %s loaded successfully",
						 plugin_full_path);
				}
				g_free (plugin_full_path);
			}
			g_dir_close (dir);
		}
		else
			return FALSE;
	}
	return TRUE;
}

/* load all modules in the paths */
static gboolean
load_module (LogviewPluginMgr *self, const gchar* module_path)
{
	LogviewModule* module = NULL;

	LV_MARK;
	if (!is_plugin_file (module_path)) {
		return FALSE;
	}

	module = logview_new_module (module_path);
	g_assert (module != NULL);
	
	if (!g_type_module_use ((GTypeModule*) module)) {
		LV_INFO ("module %s loaded failed: %s",
			 logview_module_name (module), g_module_error ());
		g_object_unref (module);
		return FALSE;
	}

	if (register_module (self, module))
		return TRUE;
	else {
		g_type_module_unuse ((GTypeModule*) module);
		return FALSE;
	}	
}

/* destroy all modules in the paths
 * must be called after destroy all plugin instances
 */
static void
unload_modules(LogviewPluginMgr *self)
{
	PluginFunc i;
	GSList* idx;
	LV_MARK;
	
	for (idx = self->prv->all_modules; idx != NULL; idx = idx->next) {
		LogviewModule *module = LOGVIEW_MODULE (idx->data);
		g_assert (module != NULL);
		LV_INFO ("unload|module: %s", logview_module_name (module));
		g_type_module_unuse ((GTypeModule*) module);
	}
	g_slist_free (self->prv->all_modules);
	self->prv->all_modules = NULL;

	for (i = 0; i < MGR_PF_NUM; i++) {
		if (self->prv->modules[i]) {
			g_slist_free (self->prv->modules[i]);
			self->prv->modules[i]  = NULL;
		}
	}
}

static gboolean
register_module (LogviewPluginMgr *self, LogviewModule* module)
{
	GType mt = logview_module_type (module);
	PluginFunc i;
	gboolean ret = FALSE;

	LV_MARK;
	for (i = 0; i < MGR_PF_NUM; i++) {
		if (g_type_is_a (mt, self->prv->typehash[i])) {
			LV_INFO ("module %s, derived from type %s",
				 logview_module_name (module),
				 g_type_name (self->prv->typehash[i]));

			if (g_slist_find_custom (self->prv->modules[i],
						 module, module_cmpfunc) == NULL) {
				self->prv->modules[i] = 
					g_slist_insert_sorted_with_data (
						self->prv->modules[i], 
						module, module_prio_cmpfunc, NULL);
				ret = TRUE;
			}
		}
	}
	if (ret) {
		self->prv->all_modules = g_slist_append (self->prv->all_modules, module);
	}
	return ret;
}

static void
unload_module (LogviewPluginMgr *self, LogviewModule* module)
{
	GType mt = logview_module_type (module);
	PluginFunc i;
	GSList* idx = NULL;
	LV_MARK;

	for (i = 0; i < MGR_PF_NUM; i++) {
		if (g_type_is_a (mt, self->prv->typehash[i])) {
			idx = g_slist_find (self->prv->modules[i], module);
			g_assert (idx != NULL);
			self->prv->modules[i] =	
				g_slist_delete_link (self->prv->modules[i], idx);
		}
	}
	idx = g_slist_find (self->prv->all_modules, module);
	g_assert (idx != NULL);
	self->prv->all_modules = g_slist_delete_link (self->prv->all_modules, idx);
	g_type_module_unuse ((GTypeModule*) module);
}

static gint
module_cmpfunc (gconstpointer a, gconstpointer b)
{
	return g_ascii_strcasecmp (logview_module_name (LOGVIEW_MODULE(a)), 
				   logview_module_name (LOGVIEW_MODULE(b)));
}

static gint
module_prio_cmpfunc (gconstpointer a, gconstpointer b, gpointer user_data)
{
	PluginPrio ap = logview_module_priority (LOGVIEW_MODULE (a));
	PluginPrio bp = logview_module_priority (LOGVIEW_MODULE (b));
	/* from high priory to low */
	return ap > bp ? -1 : (ap == bp ? 0 : 1);
}

static gboolean
is_plugin_file (const gchar* filepath)
{
	gchar* suffix[] = {
		G_MODULE_SUFFIX,
		NULL};
	gchar ext[32];
	gint i;
	for (i = 0; suffix[i] != NULL; i++) {
		g_sprintf (ext, ".%s", suffix[i]);
		if (g_str_has_suffix (filepath, ext)) {
			if (g_access (filepath, R_OK) == 0) {
				return TRUE;
			}
			return FALSE;
		}
	}
	return FALSE;
}

void
iterate (LogviewPluginMgr *self, ModuleHandleCB cb, gpointer data)
{
	GSList *idx;
	for (idx = self->prv->all_modules; idx; idx = g_slist_next(idx)) {
			cb (LOGVIEW_MODULE (idx->data), data);
	}
}

static void
pluginmgr_class_init (gpointer g_class, gpointer g_class_data)
{
	GObjectClass *object_class;
	LogviewPluginMgrClass *self = LOGVIEW_PLUGINMGR_CLASS (g_class);

	object_class = G_OBJECT_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);
	object_class->finalize = (void(*)(GObject*)) pluginmgr_finalize;

	self->load_module = load_module;
	self->unload_module = unload_module;
	self->load_modules = load_modules;
	self->unload_modules = unload_modules;
	self->iterate = iterate;

}

GSList *
pluginmgr_get_all_logs ()
{
	GSList *all_logs = NULL;
	GSList *idx = NULL;
	
	LV_MARK;
	
	for (idx = pluginmgr_instance->prv->modules[PF_LOG_COLLECTOR];
	     idx != NULL; idx = idx->next) {
		GSList *logs, *i;
		LogviewPlugin *plugin;
		g_assert (LOGVIEW_IS_MODULE (idx->data));
		plugin = logview_module_new_plugin (LOGVIEW_MODULE (idx->data));
		g_assert (LOGVIEW_IS_IFACE_COLLECTOR (plugin));
		logs = logi_get_logs (LOGVIEW_IFACE_COLLECTOR (plugin));
		g_object_unref (plugin);
		if (logs) {
			for (i = logs; i; i = g_slist_next(i)) {
				if (g_slist_find_custom (all_logs, i->data,
							 (GCompareFunc) g_ascii_strcasecmp)== NULL) {
					all_logs = g_slist_append (all_logs, i->data);
					LV_INFO ("all logs: %s", i->data);
				}
				else
					g_free (i->data);
			}
			g_slist_free (logs);
		}
	}
	
	return all_logs;
}

static void
pluginmgr_handle_derived_log (const Log* parent, PluginFunc pf, LogviewModule *module)
{
	int i;
	int versions;
	Log **older_logs;

	LV_MARK;
	g_object_get (G_OBJECT (parent),
		      "versions", &versions,
		      "archives", &older_logs,
		      NULL);
	if (versions > 0) {
		for (i=0; i<OLD_LOG_NUM; i++) {
			if (older_logs[i]) {
				LogviewPlugin *plugin;
				plugin = logview_module_new_plugin (module);
				if (logi_can_handle (LOGVIEW_IFACE (plugin),
						     older_logs[i])) {
					if (logview_plugin_init (plugin)) {
						log_set_func (older_logs[i],pf,plugin);
					}
				}
			}
		}
	}
}

/* try each plugin to find one which can handle the log */
static LogviewPlugin*
pluginmgr_probe_plugin (const Log* log, PluginFunc pf)
{
	/* do not check dependency right now */
	GSList *idx;
	LogviewPlugin *plugin;
	
	LV_MARK;
	
	if (pluginmgr_instance->prv->modules[pf] != NULL) {
		for (idx = pluginmgr_instance->prv->modules[pf]; idx != NULL; idx=idx->next) {
			LogviewModule *module;
			module = LOGVIEW_MODULE (idx->data);
			LV_ERR ("probing module %s", logview_module_name (module));
			plugin = logview_module_new_plugin (module);
			g_return_val_if_fail (plugin != NULL, NULL);
			g_return_val_if_fail (LOGVIEW_IS_PLUGIN (plugin), NULL);
			g_return_val_if_fail (LOGVIEW_IS_IFACE (plugin), NULL);
			if (logi_can_handle (LOGVIEW_IFACE (plugin), (struct _Log *) log)) {
				if (logview_plugin_init (plugin)) {
					pluginmgr_handle_derived_log (log, pf, module);
					return plugin;
				}
			}
			g_object_unref (plugin);
		}
	}
	return NULL;
}

gboolean pluginmgr_load_modules ()
{
	return LOGVIEW_PLUGINMGR_GET_CLASS (pluginmgr_instance)->load_modules (pluginmgr_instance);
}

void pluginmgr_unload_modules ()
{
	LOGVIEW_PLUGINMGR_GET_CLASS (pluginmgr_instance)->unload_modules (pluginmgr_instance);
}

Log*
pluginmgr_new_log_from_path (const gchar* log_path)
{
	Log *log = NULL;
	GSList *idx = NULL;
	LV_MARK;
	log_error_init (log_path);
	for (idx = pluginmgr_instance->prv->modules[PF_LOG_COLLECTOR]; 
	     idx != NULL; idx = idx->next) {
		LogviewPlugin *plugin;
		g_assert (LOGVIEW_IS_MODULE (idx->data));
		plugin = logview_module_new_plugin (LOGVIEW_MODULE (idx->data));
		g_assert (LOGVIEW_IS_IFACE_COLLECTOR (plugin));
		if (logi_can_handle (LOGVIEW_IFACE (plugin), log)) {
			log = logi_config_log_from_path (
				LOGVIEW_IFACE_COLLECTOR (plugin), log_path);
		}
		g_object_unref (plugin);
		if (log != NULL)
			return pluginmgr_new_log (log);
	}
	return NULL;
}

static Log*
pluginmgr_new_log (Log *log)
{
	LogviewPlugin *plugin;
	Log **older_logs;
	int i;

	LV_MARK;
	
	g_assert (log);
	g_object_get (G_OBJECT (log),
		      "archives", &older_logs,
		      NULL);
	if (! log_test_func (log, PF_LOG_IO)) {
		plugin = pluginmgr_probe_plugin (log, PF_LOG_IO);
		if (plugin != NULL) {
			log_set_func (log,PF_LOG_IO,plugin);
		}
		else {
			g_object_unref (log);
			return NULL;
		}
	}

	for (i = PF_LOG_IO + 1; i < LOG_PF_NUM; i++) {
		if (! log_test_func (log, i)) {
			plugin = pluginmgr_probe_plugin (log, i);
			if (plugin != NULL) {
				log_set_func (log,i,plugin);
			}
		}
	}

	/* Check for older versions of the log */
	for (i=0; i<OLD_LOG_NUM; i++) {
		if (older_logs[i] &&
		    LOGVIEW_IS_LOG(older_logs[i]))
			pluginmgr_new_log (older_logs[i]);
	}
	g_assert (LOGVIEW_IS_LOG (log));
	log_run (log);
	log_notify (log, TH_EVENT_INIT_READ);
	return log;
}


/* global functions */

void
pluginmgr_destroy ()
{
	LV_MARK;
	if (pluginmgr_instance != NULL) {
		g_object_unref (pluginmgr_instance);
		pluginmgr_instance = NULL;
	}
}

void
pluginmgr_init()
{
	LV_MARK;
	if (pluginmgr_instance == NULL) {
		pluginmgr_instance = g_object_new (LOGVIEW_TYPE_PLUGINMGR, NULL);
	}
}

void
pluginmgr_modules_iterate (ModuleHandleCB cb, gpointer data)
{
	LOGVIEW_PLUGINMGR_GET_CLASS (pluginmgr_instance)->iterate (pluginmgr_instance,
								   cb, data);
}

gint
pluginmgr_plugin_amount ()
{
	gint i, n;
	for (i = 0, n = 0; i < MGR_PF_NUM; i++) {
		n += g_slist_length (pluginmgr_instance->prv->modules[i]);
	}
	return n;
}
