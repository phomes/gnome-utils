/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <libgnomevfs/gnome-vfs.h>
#include "logview-iface-io.h"
#include "misc.h"
#include "logview-debug.h"
#include "logview-log.h"
#include "logview-plugin.h"

#define PLUGIN_PIPELOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PLUGIN_TYPE_PIPELOG, PluginPipelog))
#define PLUGIN_PIPELOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PLUGIN_TYPE_PIPELOG, PluginPipelogClass))
#define PIPELOG_IS_PLUGIN(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLUGIN_TYPE_PIPELOG))
#define PIPELOG_IS_PLUGIN_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLUGIN_TYPE_PIPELOG))
#define PLUGIN_PIPELOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), PLUGIN_TYPE_PIPELOG, PluginPipelogClass))

#undef PLUGIN_DESC
#define PLUGIN_DESC	"For non-ASCII formatted log files"
#undef PLUGIN_PRIO
#define PLUGIN_PRIO (PLUG_PRIO_BASE + 1)

typedef struct _PluginPipelog PluginPipelog;
typedef struct _PluginPipelogClass PluginPipelogClass;

struct _PluginPipelog {
	LogviewPlugin parent;

	Log *log;

	gchar *log_name;
	LogProtocol protocol;

	gchar *pipe;
	time_t file_time;
	GnomeVFSFileSize file_size;
	FILE *file_handle;
	gchar *pbuffer;
	gssize psize;
	gssize poffset;
};

struct _PluginPipelogClass {
	LogviewPluginClass parent;
};

static GObjectClass *parent_class = NULL;

/* default routines for implement the plugin interfaces */
static void iface_init (gpointer g_iface, gpointer iface_data);
static void io_init (gpointer g_iface, gpointer iface_data);
static gboolean plugin_init (PluginPipelog* self);
static void plugin_destroy (PluginPipelog* self);

/* interfaces */
static gboolean can_handle(PluginPipelog *self, Log *log);
static gboolean can_monitor(PluginPipelog *self);
static gboolean has_updated(PluginPipelog *self);
static gboolean update(PluginPipelog *self);
static void logf_extract_filepath (PluginPipelog *self, gchar** dirname, gchar** filename);
static size_t logf_read (PluginPipelog *self, void* buffer, size_t size);
static off_t logf_seek (PluginPipelog *self, off_t offset, int whence);
static off_t logf_tell (PluginPipelog *self);
static time_t get_modified_time (PluginPipelog* self);
static off_t get_size (PluginPipelog* self);

/* others */
static gboolean seek_pipe_log (PluginPipelog* self, const gchar *filename);

#define PIPELOG_CONF	"pipelog.conf"

/* class implement */
static void
pipelog_finalize (PluginPipelog *self)
{
	g_free (self->log_name);
}

static void
pipelog_class_init (gpointer g_class, gpointer g_class_data)
{
	LV_MARK;
	LogviewPluginClass *parent = LOGVIEW_PLUGIN_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);
	G_OBJECT_CLASS (g_class)->finalize = (void (*) (GObject*)) pipelog_finalize;

	parent->plugin_init = (gboolean (*) (LogviewPlugin*)) plugin_init;
	parent->plugin_destroy = (void (*) (LogviewPlugin*)) plugin_destroy;
}

static void
pipelog_instance_init (GTypeInstance *instance, gpointer g_class_data)
{
	PluginPipelog *self;
	LV_MARK;
	self = (PluginPipelog*) instance;
	self->pipe = NULL;
	self->pbuffer = NULL;
	self->psize = 0;
	self->poffset = 0;
	self->log_name = NULL;
}

static GType
pipelog_get_type (GTypeModule* module)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (PluginPipelogClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			pipelog_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (PluginPipelog),
			0,      /* n_preallocs */
			pipelog_instance_init    /* instance_init */
		};
		static const GInterfaceInfo iface_info = {
			iface_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		static const GInterfaceInfo io_info = {
			io_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		type = g_type_module_register_type (module,
						    LOGVIEW_TYPE_PLUGIN,
						    "PipelogType",
						    &info,
						    0);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE,
					     &iface_info);
		g_type_module_add_interface (module, type,
					     LOGVIEW_TYPE_IFACE_IO, &io_info);
	}
	return type;
}

static void load_file (PluginPipelog* self)
{
	gint inc = 5*BUFSIZ, p =0, size = 0;

	LV_MARK;
	self->pbuffer = malloc (inc * sizeof (gchar) + 1);
	while ((size = fread(self->pbuffer + p,
			     sizeof (gchar),
			     inc,
			     self->file_handle)) == inc) {
		p += inc;
		self->pbuffer = realloc (self->pbuffer, p + inc + 1);
	}
	*(self->pbuffer + p + size) = '\0';
	self->psize = p + size;
}

static gboolean
plugin_init (PluginPipelog* self)
{
	gchar *utf8_name = NULL;
	gchar *name;

	LV_MARK;
	self->file_handle = security_popen ((const char *)self->pipe, "r");
	if (self->file_handle) {
		load_file (self);
		return update (self);
	}

	if (!self->log_name) {
		/* SUN_BRANDING */
		name = _("NULL Filename");
	} else {
		utf8_name = locale_to_utf8 (self->log_name);
		name = utf8_name ? utf8_name : self->log_name;
	}
	/* SUN_BRANDING */
	log_error_appendv ("pipelog - %s - %s", name, _("popen failed"));
	g_free (utf8_name);
	return FALSE;
}

static void
plugin_destroy (PluginPipelog* self)
{
	LV_MARK;
	if (self->file_handle != NULL) {
		security_pclose (self->file_handle);
	}
	if (self->pipe)
		g_free (self->pipe);
	if (self->pbuffer)
		g_free (self->pbuffer);
}


/* interfaces */
static gboolean
can_handle(PluginPipelog *self, Log *log)
{
	gchar *utf8_name = NULL;
	gchar *name;
	gchar *message = NULL;

	LV_MARK;
	g_object_get (G_OBJECT (log),
		      "path", &self->log_name,
		      "protocol", &self->protocol,
		      NULL);

	if (self->log_name == NULL) {
		/* SUN_BRANDING */
		log_error_appendv ("pipelog - %s", _("NULL Filename"));
		return FALSE;
	}
	switch (self->protocol) {
	case UNKNOWN_LOG:
	case PLAIN_LOG:
	case PIPE_LOG:
		if (seek_pipe_log (self, self->log_name)) {
			self->log = log;
			self->protocol = PIPE_LOG;
			g_object_set (G_OBJECT (log),
				      "protocol", self->protocol,
				      NULL);
			return TRUE;
		} else {
			utf8_name = locale_to_utf8 (self->log_name);
			name = (utf8_name) ? (utf8_name) : self->log_name;
			/* SUN_BRANDING */
			log_error_appendv ("pipelog - %s - %s", name, _("Cannot open"));
			g_free (utf8_name);
			return FALSE;
		}
	default:;
	}
	utf8_name = locale_to_utf8 (self->log_name);
	name = (utf8_name) ? (utf8_name) : self->log_name;
	/* SUN_BRANDING */
	message = g_strdup_printf (_("Unknown file type: %d"), self->protocol);
	log_error_appendv ("pipelog - %s - %s", name, message);
	g_free (message);
	g_free (utf8_name);
	return FALSE;
}

static gboolean
can_monitor(PluginPipelog *self)
{
	LV_MARK;
	return FALSE;
}

static gboolean
has_updated(PluginPipelog *self)
{
	LV_MARK;
	return FALSE;
}

static gboolean
update(PluginPipelog *self)
{
	struct stat buf;
	LV_MARK;
	g_assert (self->log_name != NULL);
	if (stat (self->log_name, &buf) == 0) {
		self->file_time = buf.st_mtime;
		self->file_size = buf.st_size;
	}
	else {
		LV_INFO_EE ("%s stat: %s", self->log_name,
			    error_system_string());
		self->file_time = 0;
		self->file_size = 0;
	}
	return TRUE;
}

static size_t
logf_read (PluginPipelog *self, void* buffer, size_t size)
{
	gssize lsize = 0;
	LV_MARK;
	g_assert (size >= 0 && buffer);
	g_assert (self->psize >=0 && self->pbuffer);
	g_assert (self->poffset >=0 && self->poffset <= self->psize);
	lsize = self->psize - self->poffset;
	if (size > lsize)
		size = lsize;
	bcopy (self->pbuffer, buffer, size * sizeof(gchar));
	self->poffset += size;
	return size;
}

static off_t
logf_seek (PluginPipelog *self, off_t offset, int whence)
{
	g_assert (self->psize >=0 && self->pbuffer);
	g_assert (self->poffset >=0 && self->poffset <= self->psize);
	switch (whence) {
	case SEEK_SET:
		self->poffset = offset;
		break;
	case SEEK_CUR:
		self->poffset += offset;
		break;
	case SEEK_END:
		self->poffset = self->psize;
		break;
	default:
		return -1;
	}
	return logf_tell(self);
}

static off_t
logf_tell (PluginPipelog *self)
{
	g_assert (self->psize >=0 && self->pbuffer);
	g_assert (self->poffset >=0 && self->poffset <= self->psize);
	return (off_t)self->poffset;
}

static void
logf_extract_filepath (PluginPipelog *self, gchar** dirname, gchar** filename)
{
	extract_filepath (self->log_name, dirname, filename);
}

static off_t
get_size (PluginPipelog* self)
{
	return self->file_size;
}

static time_t
get_modified_time (PluginPipelog* self)
{
	return self->file_time;
}

static void
io_init (gpointer g_iface, gpointer iface_data)
{
	LogviewIFaceIO *iface = (LogviewIFaceIO*)g_iface;
	LV_MARK;
	iface->can_monitor = (gboolean (*) (LogviewIIO*)) can_monitor;
	iface->extract_filepath = (void (*) (LogviewIIO*, gchar**, gchar**)) logf_extract_filepath;
	iface->has_updated = (gboolean (*) (LogviewIIO*)) has_updated;
	iface->update = (void (*) (LogviewIIO*)) update;
	iface->read = (size_t (*) (LogviewIIO*, void*, size_t)) logf_read;
	iface->seek = (off_t (*) (LogviewIIO*, off_t, int)) logf_seek;
	iface->tell = (off_t (*) (LogviewIIO*)) logf_tell;
	iface->get_size = (off_t (*) (LogviewIIO*)) get_size;
	iface->get_modified_time = (time_t (*) (LogviewIIO*)) get_modified_time;
}

static void
iface_init (gpointer g_iface, gpointer iface_data)
{
	LogviewInterface *iface = (LogviewInterface*)g_iface;
	LV_MARK;
	iface->can_handle = (gboolean (*) (LogviewIFace*, Log*)) can_handle;
}

static gboolean
seek_pipe_log (PluginPipelog* self, const gchar *filename)
{
	GSList *plugin_paths, *i;
	gboolean hint = FALSE;
	gchar cbuf[BUFSIZ];
	LV_MARK;

	plugin_paths = (GSList *) get_plugin_paths ();

	for (i = plugin_paths; !hint && i; i=g_slist_next(i)) {
		gchar* path = g_build_path (G_DIR_SEPARATOR_S, i->data,
					    PIPELOG_CONF, NULL);
		if (local_file_exist (path)
		    && local_file_can_executed (path)) {
			FILE *handle = fopen (path, "r");
			if (handle != NULL) {
				while (!hint && fgets(cbuf, sizeof(cbuf), handle) != NULL) {
					gchar *str, *fp;
					g_strdelimit (cbuf, "\t\n", ' ');
					str = g_strchug (cbuf);
					if (g_str_has_prefix (str, "#"))
						continue;
					fp = g_strstr_len (str, strlen (str), filename);
					if (fp != NULL) {
						self->pipe = g_strdup_printf (
							g_strstrip(fp + strlen (filename) + 1),
							filename);
						LV_INFO ("found pipe log : %s", self->pipe);
						hint = TRUE;
					}
				}
				fclose (handle);
			}
		}
		g_free (path);
	}
	return hint;
}

LOGVIEW_INIT_PLUGIN (pipelog_get_type)

