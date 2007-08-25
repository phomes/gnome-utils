/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <signal.h>
#include <glib.h>
#include <locale.h>
#include "logview-plugin.h"
#include "logview-iface-collector.h"
#include "logview-debug.h"
#include "logview-log.h"
#include "misc.h"

#define PLUGIN_GRABLOGS(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), PLUGIN_TYPE_GRABLOGS, PluginGrabLogs))
#define PLUGIN_GRABLOGS_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), PLUGIN_TYPE_GRABLOGS, PluginGrabLogsClass))
#define GRABLOGS_IS_PLUGIN(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLUGIN_TYPE_GRABLOGS))
#define GRABLOGS_IS_PLUGIN_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), PLUGIN_TYPE_GRABLOGS))
#define PLUGIN_GRABLOGS_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), PLUGIN_TYPE_GRABLOGS, PluginGrabLogsClass))

#undef PLUGIN_PRIO
#define PLUGIN_PRIO	PLUG_PRIO_BASE
#undef PLUGIN_DESC
#define PLUGIN_DESC	"For colloct log files from shell commands"

#ifdef ON_SUN_OS
#define FILE_CMD "file"
#else
#define FILE_CMD "file"
#endif
#define GRABLOGS_CONF	"grablogs.conf"
#define CAT_CMD		"[commands]"
#define CAT_LOG		"[logs]"
#define CAT_CONF	"[configs]"


typedef GSList* (*Parse_All) (FILE*);
typedef GSList* (*Parse_Line) (FILE*, Parse_All);
typedef struct _PluginGrabLogs PluginGrabLogs;
typedef struct _PluginGrabLogsClass PluginGrabLogsClass;

struct _PluginGrabLogs {
	LogviewPlugin parent;
};

struct _PluginGrabLogsClass {
	LogviewPluginClass parent;
};

/* class implement */
static void iface_init (gpointer g_iface, gpointer iface_data);
static void collector_init (gpointer g_iface, gpointer iface_data);

static struct sigaction old_sa;
static gchar* category;

static void
grablogs_class_init (gpointer g_class, gpointer g_class_data)
{
	LV_MARK;
}

static void
grablogs_instance_init (GTypeInstance *instance, gpointer g_class_data)
{
	LV_MARK;
}

static GType
grablogs_get_type (GTypeModule* module)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (PluginGrabLogsClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			grablogs_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (PluginGrabLogs),
			0,      /* n_preallocs */
			grablogs_instance_init    /* instance_init */
		};
		static const GInterfaceInfo iface_info = {
			iface_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		static const GInterfaceInfo collector_info = {
			collector_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		type = g_type_module_register_type (module,
						    LOGVIEW_TYPE_PLUGIN,
						    "GrablogsType",
						    &info,
						    0);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE,
			&iface_info);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE_COLLECTOR,
			&collector_info);
	}
	return type;
}

static GSList*
get_logs (const gchar* logcfg, Parse_All cp)
{
	GSList* loglist = NULL;
	FILE* handle;
	int (*lclose) (FILE*) = NULL;
	
	LV_MARK;
	/*
	 * lines in [command] will be run in popen
	 */
	if (category && g_ascii_strcasecmp(g_strstrip(category),
					   CAT_CMD) == 0) {
		LV_INFO ("[grablogs-PIPE-open] %s", logcfg);
		handle = security_popen (logcfg, "r");
		lclose = security_pclose;
	}
	else if (local_file_exist (logcfg)
		 && local_file_can_executed (logcfg)) {
		LV_INFO ("[grablogs-FILE-open] %s", logcfg);
		handle = fopen (logcfg, "r");
		lclose = fclose;
	}
	else
		goto get_logs_err;

	if (handle != NULL) {
		LV_INFO ("[grablogs-open] %s", logcfg);
		loglist = cp (handle);
		if (lclose (handle) == -1)
			goto get_logs_err;
		LV_INFO ("[grablogs-close] %s", logcfg);
	}
	else {
get_logs_err:
		LV_INFO_EE ("[grablogs handles]: \"%s\", msg: %s",
			    logcfg, error_system_string ());
		return NULL;
	}
	return loglist;
}

static gchar*
get_line (FILE *handle)
{
	glong inc = BUFSIZ, size = inc;
	glong p = -1;
	gchar *buf = (gchar *) g_malloc (size * sizeof (gchar));
	g_assert (handle);
	g_assert (buf);
	do {
		g_assert (p < size);
		if (++p == size) {
			size += inc;
			buf = (gchar *) g_realloc (buf, size * sizeof(gchar));
			g_assert (buf);
		}
		buf[p] = (gchar)getc (handle);
	} while (buf[p] != '\n' && buf[p] != EOF);
	if (buf[p] != EOF) {
		buf[p] = '\0';
	} else if (feof (handle) == 0 && p > 0) {
		buf[p] = '\0';
	} else if (feof (handle) != 0) {
		LV_INFO_EE ("get_line error: %s", error_system_string());
		g_free (buf);
		buf = NULL;
	} else {
		g_free (buf);
		buf = NULL;
	}
	LV_ERR ("get_line %s", buf? buf:"EOFed");
	return buf;
}

/* one log per line */
static GSList*
cp_logline (FILE *handle)
{
	gchar *buf;
	GSList *logfiles = NULL;
	LV_MARK;
	while ((buf = get_line(handle)) != NULL) {
		if (g_str_has_prefix (g_strchug (buf), "#")) {
			g_free (buf);
			continue;
		}
		if (g_str_has_prefix(g_strstrip(buf), "[")) {
			g_free (category);
			category = buf;
			break;
		}
		if (strlen (buf) != 0)
			logfiles = g_slist_append (logfiles, g_strdup (buf));
		g_free (buf);
	}
	return logfiles;
}


static GSList*
cp_logconf (FILE *handle)
{
	gchar *buf, *p;
	GSList *logfiles = NULL;
	LV_MARK;
	while ((buf = get_line(handle)) != NULL) {
		gchar **list;
		gint i;
		for (p = buf; g_ascii_isspace(*p); ++p);
		if (*p == '\0' || *p == '#' || *p == '\n') {
			g_free (buf);
			continue;
		}
		list = g_strsplit_set (p, ", -\t()\n'`", 0);
		for (i = 0; list[i]; ++i) {
			if (*list[i] == '/' &&
			    g_slist_find_custom(logfiles, list[i],
						(GCompareFunc)g_ascii_strcasecmp) == NULL) {
				logfiles = g_slist_append (logfiles,
							   g_strdup(list[i]));
			}
		}
		g_strfreev(list);
		g_free (buf);
	}
	return logfiles;
}

/* one shell or command per line
 * run each line
 * one log per line of output
 */
static GSList*
cp_shell_per_line (FILE *handle)
{
	gchar *buf = NULL;
	GSList *all_logs = NULL;
	LV_MARK;
	while ((buf = get_line(handle)) != NULL) {
		if (g_str_has_prefix (g_strchug (buf), "#")) {
			g_free (buf);
			continue;
		}
		if (g_str_has_prefix(g_strchug (buf), "[")) {
			g_free (category);
			category = buf;
			break;
		}
		if (strlen (buf) != 0) {
			GSList *idx;
			GSList *logfiles = NULL;
			logfiles = get_logs (buf, cp_logline);
			for (idx = logfiles; idx; idx=g_slist_next(idx)) {
				if (g_slist_find_custom (all_logs, idx->data,
							 (GCompareFunc)g_ascii_strcasecmp)
				    == NULL) {
					all_logs = g_slist_append (all_logs,
								   idx->data);
				}
				else
					g_free (idx->data);
			}
			if (logfiles)
				g_slist_free (logfiles);
		}
		g_free (buf);
	}
	return all_logs;
}

static GSList*
cp_conf_per_line (FILE *handle)
{
	gchar *buf = NULL;
	GSList *all_logs = NULL;
	LV_MARK;
	while ((buf = get_line(handle)) != NULL) {
		if (g_str_has_prefix (g_strchug (buf), "#")) {
			g_free (buf);
			continue;
		}
		if (g_str_has_prefix(g_strchug (buf), "[")) {
			g_free (category);
			category = buf;
			break;
		}
		if (strlen (buf) != 0) {
			GSList *idx;
			GSList *logfiles = NULL;
			logfiles = get_logs (buf, cp_logconf);
			for (idx = logfiles; idx; idx=g_slist_next(idx)) {
				if (g_slist_find_custom (all_logs, idx->data,
							 (GCompareFunc)g_ascii_strcasecmp)
				    == NULL) {
					all_logs = g_slist_append (all_logs,
								   idx->data);
				}
				else
					g_free (idx->data);
			}
			if (logfiles)
				g_slist_free (logfiles);
		}
		g_free (buf);
	}
	return all_logs;
}

static GSList*
cp_grab_conf (FILE *handle)
{
	GSList *all_logs = NULL;
	LV_MARK;

	/* A line without any categories will be skipped */
	category = get_line(handle);
	while (feof (handle) == 0) {
		if (g_str_has_prefix (g_strchug (category), "#"))
			goto next_line;
		if (category) {
			if (g_ascii_strcasecmp(g_strstrip(category),
					       CAT_CMD) == 0) {
				LV_INFO ("grab category %s, enter", category);
				all_logs = g_slist_concat (all_logs,
							   cp_shell_per_line (handle));
			}
			else if (g_ascii_strcasecmp(g_strstrip(category),
						    CAT_LOG) == 0) {
				LV_INFO ("grab category %s, enter", category);
				all_logs = g_slist_concat (all_logs,
							   cp_logline (handle));
			}
			else if (g_ascii_strcasecmp(g_strstrip(category),
						    CAT_CONF) == 0) {
				LV_INFO ("grab category %s, enter", category);
				all_logs = g_slist_concat (all_logs,
							   cp_conf_per_line (handle));
			}
			else
				goto next_line;
		}
		else {
next_line:
			g_free (category);
			category = get_line(handle);
		}
	}
	g_free (category);
	return all_logs;
}

static GSList*
get_all_logs (PluginGrabLogs *self)
{
	GSList *plugin_paths, *i;
	GSList *all_logs = NULL;
	LV_MARK;

	plugin_paths = (GSList *) get_plugin_paths ();
	category = NULL;
	for (i = plugin_paths; i; i=g_slist_next(i)) {
		gchar* path = g_build_path (G_DIR_SEPARATOR_S,
					    i->data, GRABLOGS_CONF, NULL);
		if (local_file_exist (path)
		    && local_file_can_executed (path)) {
			GSList *logfiles, *idx;
			logfiles = get_logs (path, cp_grab_conf);
			for (idx = logfiles; idx; idx = g_slist_next(idx)) {
				if (g_slist_find_custom (all_logs, idx->data,
							 (GCompareFunc)g_ascii_strcasecmp) == NULL) {
					all_logs = g_slist_append (all_logs, idx->data);
					LV_ERR ("add %s", idx->data);
				}
				else {
					LV_ERR ("del %s", idx->data);
					g_free (idx->data);
				}
			}
			g_slist_free (logfiles);
			/* break to search the sequent path of GRABLOGS_CONF */
			g_free (path);
			break;
		} else {
			g_free (path);
		}
	}
	return all_logs;
}

static void
config_child_log (Log *log)
{
	/* Check for older versions of the log */
	gchar *log_name = NULL;
	gint i;
	g_object_get (G_OBJECT (log), "path", &log_name, NULL);
	for (i=0; i<OLD_LOG_NUM; i++) {
		gboolean flag = FALSE;
		gchar *older_name;
		Log *child;
		older_name = g_strdup_printf ("%s.%d", log_name, i);
		if (local_file_exist (older_name)) {
			flag = TRUE;
		} else {
			g_free (older_name);
			older_name = g_strdup_printf ("%s.%d.gz", log_name, i);
			if (local_file_exist (older_name)) {
				flag = TRUE;
			}
		}
		if (flag) {
			child = g_object_new (LOGVIEW_TYPE_LOG,
					      "path", older_name,
					      "protocol", file_protocol (older_name),
					      NULL);
			log_set_child (log, i, child);
		}
		g_free (older_name);
	}
	g_free (log_name);
}

static Log*
config_log_from_path (PluginGrabLogs *self, const gchar *log_path)
{
	Log *log;
	LogProtocol protocol;
	gchar *utf8_name = NULL;
	gchar *name;
	gchar *message;

	protocol = file_protocol (log_path);
	switch (protocol) {
	case UNKNOWN_LOG:
	case PLAIN_LOG:
	case GZIP_LOG:
	case PIPE_LOG:
		log = g_object_new (LOGVIEW_TYPE_LOG,
				    "path", log_path,
				    "protocol", protocol,
				    NULL);
		config_child_log (log);
		return log;
	default:;
	}
	utf8_name = locale_to_utf8 (log_path);
	name = utf8_name ? utf8_name : log_path;
	/* SUN_BRANDING */
	message = g_strdup_printf (_("Unknown file type: %d"), protocol);
	log_error_appendv ("grablogs - %s - %s", name, message);
	g_free (message);
	g_free (utf8_name);
	return NULL;
}

static void
iface_init (gpointer g_iface, gpointer iface_data)
{
	LogviewInterface *iface = (LogviewInterface*) g_iface;
	LV_MARK;
	iface->can_handle = NULL; /* dummy */
}

static void
collector_init (gpointer g_iface, gpointer iface_data)
{
	LogviewIFaceCollector *iface = (LogviewIFaceCollector*) g_iface;
	LV_MARK;
	iface->get_logs = (GSList* (*) (LogviewICollector*)) get_all_logs;
	iface->config_log_from_path = (Log* (*) (LogviewICollector*, const gchar*))config_log_from_path;
}

LOGVIEW_INIT_PLUGIN (grablogs_get_type)

