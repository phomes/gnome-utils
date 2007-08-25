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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include "logview-iface-io.h"
#include "misc.h"
#include "logview-debug.h"
#include "logview-log.h"
#include "logview-plugin.h"

#define PLUGIN_PLAINLOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PLUGIN_TYPE_PLAINLOG, PluginPlainlog))
#define PLUGIN_PLAINLOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PLUGIN_TYPE_PLAINLOG, PluginPlainlogClass))
#define PLAINLOG_IS_PLUGIN(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLUGIN_TYPE_PLAINLOG))
#define PLAINLOG_IS_PLUGIN_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLUGIN_TYPE_PLAINLOG))
#define PLUGIN_PLAINLOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), PLUGIN_TYPE_PLAINLOG, PluginPlainlogClass))

#undef PLUGIN_PRIO
#define PLUGIN_PRIO	PLUG_PRIO_BASE
#undef PLUGIN_DESC
#define PLUGIN_DESC	"For ASCII formatted log files"

typedef struct _PluginPlainLog PluginPlainLog;
typedef struct _PluginPlainLogClass PluginPlainLogClass;

struct _PluginPlainLog {
	LogviewPlugin parent;

	Log *log;

	gchar *log_name;
	LogProtocol protocol;
	
	time_t file_time;
	GnomeVFSFileSize file_size;
	GnomeVFSHandle *file_handle;
};

struct _PluginPlainLogClass {
	LogviewPluginClass parent;
};

static GObjectClass *parent_class = NULL;

/* default routines for implement the plugin interfaces */
static void iface_init (gpointer g_iface, gpointer iface_data);
static void io_init (gpointer g_iface, gpointer iface_data);
static gboolean plugin_init (PluginPlainLog* self);
static void plugin_destroy (PluginPlainLog* self);

/* interfaces */
static gboolean can_handle(PluginPlainLog *self, Log *log);
static gboolean can_monitor(PluginPlainLog *self);
static gboolean has_updated(PluginPlainLog *self);
static gboolean update(PluginPlainLog *self);
static void logf_extract_filepath (PluginPlainLog *self, gchar** dirname, gchar** filename);
static size_t logf_read (PluginPlainLog *self, void* buffer, size_t size);
static off_t logf_seek (PluginPlainLog *self, off_t offset, int whence);
static off_t logf_tell (PluginPlainLog *self);
static time_t get_modified_time (PluginPlainLog* self);
static off_t get_size (PluginPlainLog* self);

/* others */
static gboolean regular_open (PluginPlainLog *self);
static void regular_close (PluginPlainLog *self);

/* class implement */
static void
plainlog_finalize (PluginPlainLog *self)
{
	g_free (self->log_name);
}

static void
plainlog_class_init (gpointer g_class, gpointer g_class_data)
{
	LV_MARK;
	LogviewPluginClass *parent = LOGVIEW_PLUGIN_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);
	G_OBJECT_CLASS (g_class)->finalize = (void (*) (GObject*)) plainlog_finalize;

	parent->plugin_init = (gboolean (*) (LogviewPlugin*)) plugin_init;
	parent->plugin_destroy = (void (*) (LogviewPlugin*)) plugin_destroy;
}

static void
plainlog_instance_init (GTypeInstance *instance, gpointer g_class_data)
{
	PluginPlainLog *self;
	LV_MARK;
	self = (PluginPlainLog*) instance;
	self->log_name = NULL;
}

static GType
plainlog_get_type (GTypeModule* module)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (PluginPlainLogClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			plainlog_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (PluginPlainLog),
			0,      /* n_preallocs */
			plainlog_instance_init    /* instance_init */
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
						    "PlainlogType",
						    &info,
						    0);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE,
			&iface_info);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE_IO,
			&io_info);
	}
	return type;
}

static gboolean
plugin_init (PluginPlainLog* self)
{
	gchar *utf8_name = NULL;
	gchar *name;

	LV_MARK;
	if (regular_open (self))
		return update (self);
	utf8_name = locale_to_utf8 (self->log_name);
	name = utf8_name ? utf8_name : self->log_name;
	/* SUN_BRANDING */
	log_error_appendv ("plainlog - %s - %s", name, _("Cannot open"));
	g_free (utf8_name);
	return FALSE;
}

static void
plugin_destroy (PluginPlainLog* self)
{
	LV_MARK;
	if (self->file_handle != NULL) {
		regular_close (self);
	}
}

/* interfaces */
static gboolean
can_handle(PluginPlainLog *self, Log *log)
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
		log_error_appendv ("plainlog - %s", _("NULL Filename"));
		return FALSE;
	}
	if (self->protocol == PLAIN_LOG ||
	    self->protocol == GZIP_LOG) {
		self->log = log;
		return TRUE;
	} else if (self->protocol == UNKNOWN_LOG) {
		/* try to open */
		if (try_to_open (self->log_name, FALSE)) {
			self->log = log;
			self->protocol = PLAIN_LOG;
			g_object_set (G_OBJECT (log),
				      "protocol", self->protocol,
				      NULL);
			return TRUE;
		}
		else {
			utf8_name = locale_to_utf8 (self->log_name);
			name = utf8_name ? utf8_name : self->log_name;
			/* SUN_BRANDING */
			log_error_appendv ("plainlog - %s - %s", name, _("Cannot open"));
			g_free (utf8_name);
			return FALSE;
		}
	}
	utf8_name = locale_to_utf8 (self->log_name);
	name = utf8_name ? utf8_name : self->log_name;
	/* SUN_BRANDING */
	message = g_strdup_printf (_("Unknown file type: %d"), self->protocol);
	log_error_appendv ("plainlog - %s - %s", name, message);
	g_free (message);
	g_free (utf8_name);
	return FALSE;
}

static gboolean
can_monitor(PluginPlainLog *self)
{
	LV_MARK;
	switch (self->protocol) {
	case PLAIN_LOG:
		return TRUE;
	default:
		return FALSE;
	}
}

static gboolean
has_updated(PluginPlainLog *self)
{
	struct stat buf;
	LV_MARK;

	if (stat (self->log_name, &buf) == 0) {
		if (buf.st_mtime != self->file_time) {
			return TRUE;
		}
	} else {
		LV_INFO_EE ("%s stat: %s", self->log_name,
			    error_system_string());
	}
	return FALSE;
}

static gboolean
update(PluginPlainLog *self)
{
	struct stat buf;
	LV_MARK;
	g_assert (self->log_name != NULL);
	if (stat (self->log_name, &buf) == 0) {
		self->file_time = buf.st_mtime;
		self->file_size = buf.st_size;
		return TRUE;
	} else {
		LV_INFO_EE ("%s stat: %s", self->log_name,
			    error_system_string());
	}
	return FALSE;
}

static size_t
logf_read (PluginPlainLog *self, void* buffer, size_t size)
{
	GnomeVFSResult result;
	GnomeVFSFileSize read_size;
	
	LV_MARK;
	g_return_val_if_fail (self->file_handle !=NULL, -1);
	result = gnome_vfs_read (self->file_handle,
				 buffer,
				 (GnomeVFSFileSize)size,
				 &read_size);
	if ( result != GNOME_VFS_OK) {
		LV_INFO_EE ("%s: (%d), %s",
			    self->log_name,
			    result, gnome_vfs_result_to_string (result));
		return -1;
	}
	return (size_t)read_size;
}

static off_t
logf_seek (PluginPlainLog *self, off_t offset, int whence)
{
	GnomeVFSSeekPosition sp;
	switch (whence) {
	case SEEK_SET:
		sp = GNOME_VFS_SEEK_START;
		break;
	case SEEK_CUR:
		sp = GNOME_VFS_SEEK_CURRENT;
		break;
	case SEEK_END:
		sp = GNOME_VFS_SEEK_END;
		break;
	default:
		return -1;
	}
	if (gnome_vfs_seek (self->file_handle, sp, (GnomeVFSFileOffset)offset) == GNOME_VFS_OK) {
		return logf_tell(self);
	}
	else
		return -1;
}

static off_t
logf_tell (PluginPlainLog *self)
{
	GnomeVFSFileSize size;
	if (gnome_vfs_tell (self->file_handle, &size) == GNOME_VFS_OK) {
		return (off_t)size;
	}
	return -1;
}

static void
logf_extract_filepath (PluginPlainLog *self, gchar** dirname, gchar** filename)
{
	extract_filepath (self->log_name, dirname, filename);
}

static off_t
get_size (PluginPlainLog* self)
{
	return self->file_size;
}

static time_t
get_modified_time (PluginPlainLog* self)
{
	return self->file_time;
}

static void
io_init (gpointer g_iface, gpointer iface_data)
{
	LogviewIFaceIO *iface = (LogviewIFaceIO*) g_iface;
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

/* log functions */
static gboolean
regular_open (PluginPlainLog *self)
{
	char *file_name;
	GnomeVFSResult result;

	if (self->protocol == GZIP_LOG) {
		file_name = g_strdup_printf ("%s#gzip:", self->log_name);
	}
	else {
		file_name = g_strdup (self->log_name);
	}
	result = gnome_vfs_open (&self->file_handle,
				 file_name, GNOME_VFS_OPEN_READ);
	g_free (file_name);
	if (result != GNOME_VFS_OK) {
		LV_INFO_EE ("%s: (%d), %s",
			    self->log_name,
			    result, gnome_vfs_result_to_string (result));
		self->file_handle = NULL;
		return FALSE;
	}
	return TRUE;
}

static void
regular_close (PluginPlainLog *self)
{
	GnomeVFSResult result;

	result = gnome_vfs_close (self->file_handle);
	if (result != GNOME_VFS_OK) {
		LV_INFO_EE ("%s: (%d), %s",
			    self->log_name,
			    result, gnome_vfs_result_to_string (result));
	}
}

LOGVIEW_INIT_PLUGIN (plainlog_get_type)
