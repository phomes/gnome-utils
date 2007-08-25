/*	----------------------------------------------------------------------

	Copyright (C) 1998	Cesar Miquel  (miquel@df.uba.ar)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	---------------------------------------------------------------------- */

#ifdef __CYGWIN__
#define timezonevar
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include "logview.h"
#include "logview-log.h"
#include "logrtns.h"
#include "log_repaint.h"
#include "misc.h"
#include "math.h"
#include "logview-iface-io.h"
#include "logview-iface-view.h"
#include "logview-plugin-manager.h"
#include "logview-debug.h"

#define LINES_INIT_ALLOC	128
#define LINES_STEP_ALLOC_MUTI_FACTOR	2
#define BOLD_ROWS_TIME		5000
#define WRITE_LINES		20
#define DATA_LOCK(log)		g_mutex_lock(log->prv->data_mutex)
#define DATA_UNLOCK(log)	g_mutex_unlock(log->prv->data_mutex)

#define LOG_IS_VALID(log)	g_assert (log); \
				g_assert (LOGVIEW_IS_LOG (log)); \
				g_assert (log->prv->dispose_has_run == FALSE)

static void log_dispose (Log *log);
static void log_finalize (Log *log);
static void log_thread_idle (Log *self);
static guint log_thread_main_loop (Log *self);
static ThreadEvent log_thread_pop_event (Log *self);
static void log_create_model (Log *log);
static void log_thread_fill_content (Log *log);
static void log_fill_model_with_date (Log *log, GSList *days);
static void log_fill_model_no_date (Log *log, guint start, guint end);
static gboolean log_unbold_rows (Log *log);

static GObjectClass *parent_class = NULL;

enum {
	PROP_0 = 0,
	PROP_NAME,
	PROP_PROTOCOL,
	PROP_MODEL,
	PROP_FILTER,
	PROP_DISPLAY_NAME,
	PROP_TOTAL_LINES,
	PROP_IS_MONITORED,
	PROP_PARENT,
	PROP_VERSIONS,
	PROP_CURRENT_VERSION,
	PROP_ARCHIVES,
	PROP_DAYS,
	PROP_WINDOW,
	PROP_VISIBLE_FIRST,
	PROP_BOLD_FIRST,
	PROP_BOLD_LAST,
        PROP_SELECTED_PATHS,
	PROP_SIZE,
	PROP_MTIME,
	PROP_MONITORABLE,
	PROP_MONITORING,
	PROP_MONITOR_HANDLE,
	PROP_GROUPABLE
};

struct _LogPrivate {
	gboolean dispose_has_run;

	/* event related */
	volatile ThreadEvent th_event;
	GMutex* thread_lock;
	GMutex* data_mutex;
	GMutex* event_mutex;
	GCond* event_cond;

	gchar *log_path;
	LogProtocol protocol;

	/* content information */
	GSList *need_free;
	gchar **lines;
	guint total_lines; /* current recorded lines in the file */
	guint alloc_lines; /* capability of max lines in the file */
	
	/* isolated calendar */
	GList *selected_paths;

	/* isolataed repaint */
	GtkTreePath *first_bold_row;
	GtkTreePath *last_bold_row;
	GtkTreeModel *model;
	gint current_version;
	GtkTreeModelFilter *filter;
	GSList *days;
	/* isolated logview */
	gchar *display_name;
	gint versions;
	Log *parent_log;	
	GtkTreePath *visible_first;

	Log **older_logs;
	
	/* Monitor info */
	gboolean monitored;
	GnomeVFSMonitorHandle *mon_handle;

	/* other informations */
	gpointer window;
	/* plugin */
	LogviewPlugin* ext_data[LOG_PF_NUM];

	/* write pointer */
	GtkTreeIter *date_iter;
	GtkTreeIter *line_iter;

	/* status */
	gboolean update_status;
};

static void
log_instance_init (GTypeInstance *instance, gpointer g_class_data)
{
	Log* self = (Log*) instance;
	self->prv = g_new0(LogPrivate, 1);
	self->prv->dispose_has_run = FALSE;

	/* thread related */
	self->prv->thread_lock = g_mutex_new ();
	g_assert (self->prv->thread_lock);
	self->prv->data_mutex = g_mutex_new ();
	g_assert (self->prv->data_mutex);
	self->prv->event_mutex = g_mutex_new ();
	g_assert (self->prv->event_mutex);
	self->prv->event_cond = g_cond_new ();
	g_assert (self->prv->event_cond);
	self->prv->th_event = TH_EVENT_EMPTY;

	self->prv->alloc_lines = LINES_INIT_ALLOC;
	self->prv->lines = g_malloc0 (LINES_INIT_ALLOC * sizeof (gchar *));
	g_assert (self->prv->lines);
	self->prv->older_logs = g_malloc0 (OLD_LOG_NUM * sizeof (Log *));
	g_assert (self->prv->older_logs);
	self->prv->lines[0] = NULL;
	self->prv->line_iter = g_new0 (GtkTreeIter,1);
}

static void
log_set_property (GObject      *object,
		  guint         property_id,
		  const GValue *value,
		  GParamSpec   *pspec)
{
	Log *self = LOGVIEW_LOG (object);

	LOG_IS_VALID(self);

	DATA_LOCK (self);
	switch (property_id) {
	case PROP_NAME:
		g_free (self->prv->log_path);
		self->prv->log_path = g_value_dup_string (value);
		g_assert (self->prv->log_path);
		g_assert (self->prv->log_path[0] != '\0');
		LV_INFO ("log.path: %s",self->prv->log_path);
		break;
	case PROP_PROTOCOL:
		self->prv->protocol = g_value_get_ulong (value);
		LV_INFO ("log.protocol: %d",self->prv->protocol);
		break;
	case PROP_CURRENT_VERSION:
		self->prv->current_version = g_value_get_int (value);
		break;
	case PROP_VERSIONS:
		self->prv->versions = g_value_get_int (value);
		LV_INFO ("log.versions: %d",self->prv->versions);
		break;
	case PROP_VISIBLE_FIRST:
		if (self->prv->visible_first)
			gtk_tree_path_free (self->prv->visible_first);
		self->prv->visible_first = g_value_get_pointer (value);
		break;
	case PROP_SELECTED_PATHS: {
		GList *paths;
		paths = g_value_get_pointer (value);
		g_assert (paths);
		if (self->prv->selected_paths != paths) {
			g_list_foreach (self->prv->selected_paths, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (self->prv->selected_paths);
			self->prv->selected_paths = paths;
		}
	}
		break;
	case PROP_WINDOW:
		/* can be set only once */
		g_assert (self->prv->window == NULL);
		self->prv->window = g_value_get_pointer (value);
		break;
	case PROP_MONITOR_HANDLE:
		self->prv->mon_handle = g_value_get_pointer (value);
		break;
	case PROP_MONITORING:
		self->prv->monitored = g_value_get_boolean (value);
		break;
	case PROP_FILTER:
		self->prv->filter = g_value_get_pointer (value);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
	DATA_UNLOCK (self);
}

static void
log_get_property (GObject      *object,
		  guint         property_id,
		  GValue       *value,
		  GParamSpec   *pspec)
{
	Log *self = LOGVIEW_LOG (object);

	LOG_IS_VALID(self);

	DATA_LOCK (self);
	switch (property_id) {
	case PROP_NAME:
		g_value_set_static_string (value, self->prv->log_path);
		break;
	case PROP_PROTOCOL:
		g_value_set_ulong (value, self->prv->protocol);
		break;
	case PROP_MODEL:
		g_value_set_pointer (value, self->prv->model);
		break;
	case PROP_PARENT:
		g_value_set_object (value, self->prv->parent_log);
		break;
	case PROP_FILTER:
		g_value_set_pointer (value, self->prv->filter);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value, self->prv->display_name);
		break;
	case PROP_TOTAL_LINES:
		g_value_set_uint (value, self->prv->total_lines);
		break;
	case PROP_MONITORABLE:
		g_value_set_boolean (value, logi_can_monitor(LOGVIEW_IFACE_IO(log_io(self))));
		break;
	case PROP_MONITORING:
		g_value_set_boolean (value, self->prv->monitored);
		break;
	case PROP_GROUPABLE: {
		gboolean ret;
		ret = self->prv->days != NULL;
		g_value_set_boolean (value, ret);
	}
		break;
	case PROP_VISIBLE_FIRST:
		g_value_set_pointer (value, self->prv->visible_first);
		break;
	case PROP_BOLD_FIRST:
		g_value_set_pointer (value, self->prv->first_bold_row);
		break;
	case PROP_BOLD_LAST:
		g_value_set_pointer (value, self->prv->last_bold_row);
		break;
	case PROP_VERSIONS:
		g_value_set_int (value, self->prv->versions);
		break;
	case PROP_ARCHIVES:
		g_value_set_pointer (value, self->prv->older_logs);
		break;
	case PROP_CURRENT_VERSION:
		g_value_set_int (value, self->prv->current_version);
		break;
	case PROP_DAYS:
		g_value_set_pointer (value, self->prv->days);
		break;
	case PROP_WINDOW:
		g_value_set_pointer (value, self->prv->window);
		break;
	case PROP_SELECTED_PATHS:
		g_value_set_pointer (value, self->prv->selected_paths);
		break;
	case PROP_SIZE: {
		guint64 size = (guint64) logi_get_size (LOGVIEW_IFACE_IO(log_io(self)));
		g_value_set_uint64 (value, size);
		break;
	}
	case PROP_MTIME: {
		time_t time = logi_get_modified_time(LOGVIEW_IFACE_IO(log_io(self)));
		g_value_set_long (value, time);
		break;
	}
	case PROP_MONITOR_HANDLE:
		g_value_set_pointer (value, self->prv->mon_handle);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
	DATA_UNLOCK (self);
}

static void
log_class_init (gpointer g_class, gpointer g_class_data)
{
	GObjectClass *object_class;
	LogClass *self;
	GParamSpec *pspec;
	
	LV_MARK;

	object_class = G_OBJECT_CLASS (g_class);
	self = LOGVIEW_LOG_CLASS (g_class);
	parent_class = g_type_class_peek_parent (g_class);

        object_class->set_property = log_set_property;
	object_class->get_property = log_get_property;
	object_class->dispose = (void (*) (GObject*)) log_dispose;
	object_class->finalize = (void (*) (GObject*)) log_finalize;

	pspec = g_param_spec_string ("path",
				     "Log arbitrary path",
				     "Set or get log path",
				     "" /* default value */,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 pspec);

	pspec = g_param_spec_string ("display-name",
				     "Log display name",
				     "Set or get log display name",
				     NULL /* default value */,
				     G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 pspec);

	pspec = g_param_spec_ulong ("protocol",
				    "Log protocol",
				    "Set or get log protocol",
				    0,
				    G_TYPE_ULONG,
				    UNKNOWN_LOG /* default value */,
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class,
					 PROP_PROTOCOL,
					 pspec);

	pspec = g_param_spec_int ("current-version",
				  "Currently displaying log",
				  "Set or get current version",
				  0,
				  OLD_LOG_NUM,
				  0 /* default value */,
				  G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_CURRENT_VERSION,
					 pspec);

	pspec = g_param_spec_int ("versions",
				  "Log archive numbers",
				  "Get log archive numbers",
				  0,
				  OLD_LOG_NUM,
				  0 /* default value */,
				  G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_VERSIONS,
					 pspec);

        pspec = g_param_spec_pointer ("archives",
				      "elder logs pointer",
				      "Get elder logs pointer",
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_ARCHIVES,
					 pspec);

	pspec = g_param_spec_uint64 ("size",
				   "Log file size",
				   "Get log size",
				   0,
				   G_MAXUINT64,
				   0 /* default value */,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_SIZE,
					 pspec);

	pspec = g_param_spec_uint ("total-lines",
				   "Log current read lines",
				   "Get current read lines",
				   0,
				   G_MAXUINT,
				   0 /* default value */,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_TOTAL_LINES,
					 pspec);

	pspec = g_param_spec_long ("mtime",
				   "Log modified time",
				   "Get modified time",
				   0,
				   G_MAXLONG,
				   0 /* default value */,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_MTIME,
					 pspec);

	pspec = g_param_spec_boolean ("monitorable",
				      "Log monitor capable",
				      "Get or set monitor capable",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_MONITORABLE,
					 pspec);

	pspec = g_param_spec_boolean ("monitoring",
				      "Is monitor the log",
				      "Get monitor status",
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_MONITORING,
					 pspec);

	pspec = g_param_spec_boolean ("groupable",
				      "Log group capable",
				      "Get group capable",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_GROUPABLE,
					 pspec);

	pspec = g_param_spec_pointer ("model",
				      "Log tree model",
				      "Get log model",
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_MODEL,
					 pspec);

	pspec = g_param_spec_pointer ("filter",
				      "Log tree filter",
				      "Get log filter",
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_FILTER,
					 pspec);

	pspec = g_param_spec_object ("parent",
				     "The parent log of the current log",
				     "Get parent log",
				     LOGVIEW_TYPE_LOG,
				     G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_PARENT,
					 pspec);

	pspec = g_param_spec_pointer ("days",
				      "The day list of a log",
				      "Get log days",
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DAYS,
					 pspec);

	pspec = g_param_spec_pointer ("window",
				      "Point to the main window",
				      "Get and set window",
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class,
					 PROP_WINDOW,
					 pspec);

	pspec = g_param_spec_pointer ("visible-first",
				      "Log visible first",
				      "Set or get first visible line",
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_VISIBLE_FIRST,
					 pspec);

	pspec = g_param_spec_pointer ("first-bold-row",
				      "first bold row",
				      "Get first bold row",
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_BOLD_FIRST,
					 pspec);

	pspec = g_param_spec_pointer ("last-bold-row",
				      "last bold row",
				      "Get last bold row",
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_BOLD_LAST,
					 pspec);

	pspec = g_param_spec_pointer ("selected-paths",
				      "Log last selected lines",
				      "Set or get last selected lines",
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_SELECTED_PATHS,
					 pspec);


	pspec = g_param_spec_pointer ("monitor-handle",
				      "GnomeVFSMonitorHandle used for monitoring this log",
				      "Set or get last selected line",
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_MONITOR_HANDLE,
					 pspec);

}

GType
log_get_type (void)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			log_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (Log),
			0,      /* n_preallocs */
			log_instance_init    /* instance_init */
		};		
		type = g_type_register_static (G_TYPE_OBJECT,
					       "LogviewLogType",
					       &info, 0);
	}
	return type;
}

static void
log_append_garbage (Log *self, gchar *mass)
{
	LOG_IS_VALID(self);

	self->prv->need_free = g_slist_append (self->prv->need_free, mass);
}

static void
log_append_lines (Log *self, gchar **lines, gint len)
{
	guint i;

	LOG_IS_VALID(self);

	if (self->prv->total_lines + len >= (self->prv->alloc_lines -1)) {
		self->prv->alloc_lines *= LINES_STEP_ALLOC_MUTI_FACTOR;
		self->prv->lines = g_realloc (self->prv->lines,
					 self->prv->alloc_lines * sizeof (gchar *));
		g_assert (self->prv->lines);
	}
	for (i = 0; i < len; i++) {
		g_assert (lines[i]);
		self->prv->lines[self->prv->total_lines++] = lines[i];
	}
	self->prv->lines[self->prv->total_lines] = NULL;
}

static void
cleanup_binary_chars_in_log (gchar* buffer, gssize read_size)
{
	int i;
	for ( i = 0; i < read_size; i++ ) {
		if ( buffer[i] == '\0' ) {
			buffer[i] = '?';
		}
	}
}

/* log functions */
Day *
log_find_day (Log *self, const GDate* date)
{
	GSList *days;
	Day *day, *found_day = NULL;

	LOG_IS_VALID(self);

	DATA_LOCK (self);
	for (days = self->prv->days; days!=NULL; days=g_slist_next(days)) {
		day = days->data;
		if (g_date_compare (day->date, date) == 0) {
			found_day = day;
		}
	}
	DATA_UNLOCK (self);
	return found_day;
}

/* return start index of lines */
static guint
log_build_list_index_from_mass (Log *self, const gchar* buffer)
{
	gchar *fore_idx, *rear_idx;
	guint ret = self->prv->total_lines;
	gchar *cache[LINES_INIT_ALLOC];
	gint i = 0;

	LV_MARK;
	LOG_IS_VALID(self);
        g_assert (buffer);

	DATA_LOCK (self);
	log_append_garbage (self, (gchar *) buffer);
	for (rear_idx = fore_idx = (gchar *) buffer; *fore_idx != '\0'; fore_idx++) {
		/* do not skip empty lines */
		while (*fore_idx == '\n') {
			*fore_idx = '\0';
			if (rear_idx != fore_idx) {
				cache[i++] = rear_idx;
				if (i == LINES_INIT_ALLOC) {
					log_append_lines (self, cache, i);
					i = 0;
					DATA_UNLOCK (self);
					log_thread_idle (self);
					DATA_LOCK (self);
				}
			}
			rear_idx =  fore_idx + 1;
		}
	}
	if (i > 0)
		log_append_lines (self, cache, i);
	DATA_UNLOCK (self);
	return ret;
}

/**
 * log_read:
 * @self: a #Log instance.
 * 
 * If read end of line or zero chars or meats error, return NULL.
 * 
 * Returns: the pointer of buffer.
 */
gchar *
log_read (Log *self)
{
	gchar *buffer;
	off_t len, read_size, fp;

	LV_MARK;
	LOG_IS_VALID(self);

	g_assert (LOGVIEW_IS_IFACE_IO(log_io (self)));
	g_assert (LOGVIEW_IS_IFACE_VIEW(log_view (self)));
	DATA_LOCK (self);

	fp = logi_tell (log_io(self));
	logi_seek (log_io(self), 0, SEEK_END);
	len = logi_tell (log_io(self)) - fp;
	buffer = g_malloc (len * sizeof(gchar) +1);
	logi_seek (log_io(self), fp, SEEK_SET);
	read_size = logi_read (log_io(self), buffer, len);
        if (read_size > 0) {
		logi_update (log_io(self));
		buffer [read_size] = '\0';
		buffer = logi_to_utf8 (log_view (self), buffer, read_size);
		cleanup_binary_chars_in_log (buffer, read_size);
        }
	else {
                /* read_size == 0 || -1, reading finished or error? */
		g_free (buffer);
		buffer = NULL;
	}
	DATA_UNLOCK (self);
	return buffer;
}

static guint
log_thread_main_loop (Log *self)
{
	ThreadEvent te;

	LOG_IS_VALID(self);

	/* lock thread, prevent free instance before exist from thread */
	g_mutex_lock (self->prv->thread_lock);
	while ((te = log_thread_pop_event (self))) {
		switch (te) {
		case TH_EVENT_INIT_READ:
			self->prv->update_status = FALSE;
			LV_INFO ("%s begin building log lines...",
				 self->prv->log_path);
			log_thread_fill_content (self);
			LV_INFO ("%s finished building log lines,"
				 "found %ld new lines",
				 self->prv->log_path,
				 self->prv->total_lines);
			break;
		case TH_EVENT_UPDATE:
			self->prv->update_status = TRUE;
			log_thread_fill_content (self);
			break;
		case TH_EVENT_EXIT:
			goto thread_exit;
			break;
		default:
			g_assert_not_reached ();
		}
	}
thread_exit:
	g_mutex_unlock (self->prv->thread_lock);
	return 0;
}

gboolean
log_run (Log *self)
{
	/* this function should only be ran once */
	LOG_IS_VALID(self);
	g_assert (self->prv->model == NULL);

	log_create_model (self);
	LV_INFO ("[[Log thread run]] %s", self->prv->log_path);
	if (g_thread_create ((GThreadFunc) log_thread_main_loop, self, TRUE, NULL))
		return TRUE;
	else {
		LV_INFO ("create thread failed", NULL);
		return FALSE;
	}
}

static void
log_dispose (Log *self)
{
	LV_MARK;
	if (self->prv->dispose_has_run) {
		/* If dispose did already run, return. */
		return;
	}
	self->prv->dispose_has_run = TRUE;
	G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (self));
}

static void
log_finalize (Log *self)
{
	gint i;
	GSList *idx;
	Day *day;
	GSList *days;

	LV_MARK;
	LV_INFO ("[[ log remove ]] %s", self->prv->log_path);

	log_notify (self, TH_EVENT_EXIT);
	/* make sure thread existed */
	g_mutex_lock (self->prv->thread_lock);
	g_mutex_unlock (self->prv->thread_lock);
	g_mutex_free (self->prv->thread_lock);
	self->prv->thread_lock = NULL;
	/* free other locks */
	g_assert (LOGVIEW_IS_LOG (self));
	g_cond_free (self->prv->event_cond);
	self->prv->event_cond = NULL;
	g_mutex_free (self->prv->event_mutex);
	self->prv->event_mutex = NULL;
	g_mutex_free (self->prv->data_mutex);
	self->prv->data_mutex = NULL;
	/* Close archive logs if there's some */
	for (i = 0; i < self->prv->versions; i++)
		if (self->prv->older_logs[i])
			g_object_unref (self->prv->older_logs[i]);
	g_free (self->prv->older_logs);

	if (self->prv->mon_handle != NULL) {
		gnome_vfs_monitor_cancel (self->prv->mon_handle);
		self->prv->mon_handle = NULL;
	}

	if (self->prv->model) {
		gtk_tree_store_clear (GTK_TREE_STORE (self->prv->model));
		g_object_unref (self->prv->model);
	}

	if (self->prv->days != NULL) {
		for (days = self->prv->days; days != NULL; days = g_slist_next (days)) {
			day = days->data;
			g_date_free (day->date);
			gtk_tree_path_free (day->path);
			g_free (day);
		}
		g_slist_free (self->prv->days);
		self->prv->days = NULL;
	}

	if (self->prv->date_iter)
		gtk_tree_iter_free (self->prv->date_iter);
	if (self->prv->line_iter)
		gtk_tree_iter_free (self->prv->line_iter);
	if (self->prv->visible_first)
		gtk_tree_path_free (self->prv->visible_first);

	if (self->prv->first_bold_row)
		gtk_tree_path_free (self->prv->first_bold_row);
	if (self->prv->last_bold_row)
		gtk_tree_path_free (self->prv->last_bold_row);

	for (i = 0; i < LOG_PF_NUM; i++) {
		if (self->prv->ext_data[i] != NULL) {
			g_object_unref (G_OBJECT (self->prv->ext_data[i]));
		}
	}

	for (idx = self->prv->need_free; idx; idx = g_slist_next(idx)) {
		gchar *mass = (gchar *)idx->data;
		g_free (mass);
	}

	g_free (self->prv->lines);
	self->prv->lines = NULL;

	if (self->prv->display_name)
		g_free (self->prv->display_name);
	if (self->prv->log_path)
		g_free (self->prv->log_path);
	g_free (self->prv);
	self->prv = NULL;
	parent_class->finalize (G_OBJECT (self));
}

void
log_extract_filepath (Log* self, gchar** dirname, gchar** filename)
{
	LOG_IS_VALID(self);
	g_assert (LOGVIEW_IS_IFACE_IO(log_io(self)));

	logi_extract_filepath(LOGVIEW_IFACE_IO(log_io(self)), dirname, filename);
}

gboolean
log_has_been_modified (Log *self)
{
	LOG_IS_VALID(self);
	g_assert (LOGVIEW_IS_IFACE_IO(log_io(self)));

	return logi_has_updated(LOGVIEW_IFACE_IO(log_io(self)));
}

gboolean
log_test_func (Log *self, PluginFunc pf)
{
	LOG_IS_VALID(self);

	return self->prv->ext_data[pf] != NULL;
}

void
log_set_func (Log *self, PluginFunc pf, LogviewPlugin *plugin)
{
	LOG_IS_VALID(self);
	g_assert (plugin != NULL);

	self->prv->ext_data[pf] = plugin;
}

void
log_set_child (Log *self, gint index, Log *child)
{
	LOG_IS_VALID(self);
	g_assert (index >=0 && index < OLD_LOG_NUM);
	g_assert (child && LOGVIEW_IS_LOG (child));
	g_assert (self->prv->older_logs[index] == NULL);

	self->prv->older_logs[index] = child;
	self->prv->versions++;
	child->prv->parent_log = self;
}

Log*
log_get_child (Log *self, gint index)
{
	LOG_IS_VALID(self);
	g_assert (index >=0 && index < OLD_LOG_NUM);

	return self->prv->older_logs[index];
}

void
log_notify (Log *self, ThreadEvent te)
{
	g_mutex_lock (self->prv->event_mutex);
	self->prv->th_event |= te;
	g_cond_signal (self->prv->event_cond);
	g_mutex_unlock (self->prv->event_mutex);
}

static ThreadEvent
log_thread_pop_event (Log *self)
{
	ThreadEvent te;
	LOG_IS_VALID(self);

	g_mutex_lock (self->prv->event_mutex);
	while (self->prv->th_event == TH_EVENT_EMPTY)
		g_cond_wait (self->prv->event_cond, self->prv->event_mutex);
	if (self->prv->th_event & TH_EVENT_EXIT)
		te =  TH_EVENT_EXIT;
	else if (self->prv->th_event & TH_EVENT_INIT_READ)
		te =  TH_EVENT_INIT_READ;
	else if (self->prv->th_event & TH_EVENT_UPDATE)
		te =  TH_EVENT_UPDATE;
	else
		g_assert_not_reached ();
	self->prv->th_event ^= te;
	g_mutex_unlock (self->prv->event_mutex);
	return te;
}

static void
log_thread_idle (Log *self)
{
	g_assert (self);
	g_assert (LOGVIEW_IS_LOG (self));

	g_usleep (1);
	g_mutex_lock (self->prv->event_mutex);
	if (self->prv->th_event & TH_EVENT_EXIT) {
		g_mutex_unlock (self->prv->event_mutex);
		g_mutex_unlock (self->prv->thread_lock);
		g_thread_exit (0);
	}
	g_mutex_unlock (self->prv->event_mutex);
}

static void
log_create_model (Log *self)
{
	LOG_IS_VALID(self);
	self->prv->model = GTK_TREE_MODEL(
		gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_POINTER,
				    G_TYPE_INT, G_TYPE_BOOLEAN)
		);
}  

static void
log_thread_fill_content (Log *self)
{
	GSList *days, *idx;
	GtkTreePath *path = NULL;
	gchar *buffer;
	guint start, end;
	LV_MARK;

	LOG_IS_VALID(self);
	g_assert (self);

	if (self->prv->update_status) {
		if (log_has_been_modified (self) == FALSE)
			return;
	}

	if ((buffer = log_read (self)) == NULL)
                return;
	g_assert (buffer && buffer[0] != '\0');

	start = log_build_list_index_from_mass (self, buffer);
	g_assert (start <= self->prv->total_lines);

	days = logi_group_lines (log_view (self),
				 (const gchar **) (self->prv->lines + start));
	end = self->prv->total_lines;

	if (days) {
		log_fill_model_with_date (self, days);
		path = gtk_tree_model_get_path (self->prv->model,
						self->prv->date_iter);
		DATA_LOCK (self);
		self->prv->days = g_slist_concat (self->prv->days, days);
		DATA_UNLOCK (self);
	}
	else {
		GtkTreeIter iter;
		log_write_lines_range (self, self->prv->lines, start, end);
		path = gtk_tree_model_get_path (self->prv->model,
						self->prv->line_iter);
	}

	if (self->prv->update_status) {
		/* update */
		/* Remember the last bold lines in the model to 
		   unset them later */
		DATA_LOCK (self);
		if (self->prv->last_bold_row)
			gtk_tree_path_free (self->prv->last_bold_row);
		self->prv->last_bold_row =
			gtk_tree_model_get_path (self->prv->model,
						 self->prv->line_iter);
		g_timeout_add (BOLD_ROWS_TIME,
			       (GSourceFunc) log_unbold_rows,
			       self);
		DATA_UNLOCK (self);
	}
	else {
		/* initial, remember the first visible line
		   and selected lines */
		DATA_LOCK (self);
		self->prv->selected_paths =
			g_list_append (self->prv->selected_paths,
				       gtk_tree_path_copy (path));
		if (self->prv->visible_first)
			gtk_tree_path_free (self->prv->visible_first);
		self->prv->visible_first = gtk_tree_path_copy (path);
		DATA_UNLOCK (self);
	}
	gtk_tree_path_free (path);
}
    
static void
log_fill_model_with_date (Log *self, GSList *days)
{
	gint i;
	GSList *iter;
	Day *day = NULL;

	LOG_IS_VALID(self);
	g_assert (days);

	for (iter = days; iter; iter = g_slist_next (iter)) {
		day = (Day *)iter->data;

		log_write_day (self, day);
		day->path = gtk_tree_model_get_path (self->prv->model, self->prv->date_iter);
		log_write_lines_range (self, self->prv->lines,
				       day->first_line,
				       day->last_line + 1);
		day->expand = FALSE;
	}
	day->expand = TRUE;
}

void
log_write_day (Log *self, Day *day)
{
	gchar *old_date_str = NULL;
	gchar *new_date_str = NULL;
	gint old_date_len;
	gint new_date_len;
	GDate *date;

	LOG_IS_VALID(self);
        g_assert (day);
        g_assert (day->date);
        g_assert (self->prv->model);

	date = day->date;

        g_assert (g_date_valid (date));

        new_date_str = date_to_string (date); /* convert to utf8 */
        g_assert (new_date_str);

	if (self->prv->date_iter == NULL) {
		self->prv->date_iter = g_new0 (GtkTreeIter,1);
		goto log_write_date_first_run;
	}
	
        gtk_tree_model_get (self->prv->model,
                            self->prv->date_iter,
                            MESSAGE, &old_date_str,
                            -1);
        old_date_len = (gint) g_utf8_strlen (old_date_str, -1);
        new_date_len = (gint) g_utf8_strlen (new_date_str, -1);
        if ((old_date_len != new_date_len)
            || (old_date_len >= new_date_len ?
                g_ascii_strncasecmp (old_date_str, new_date_str, new_date_len)
                : g_ascii_strncasecmp (old_date_str, new_date_str, old_date_len)) != 0) {
                /* new date */
log_write_date_first_run:
                gtk_tree_store_append (GTK_TREE_STORE (self->prv->model),
                                       self->prv->date_iter,
                                       NULL);
                gtk_tree_store_set (GTK_TREE_STORE (self->prv->model),
                                    self->prv->date_iter,
                                    MESSAGE, new_date_str,
                                    DAY_POINTER, day,
                                    -1);
        }
        g_free (new_date_str);
        g_free (old_date_str);
	log_thread_idle (self);
}

void
log_write_date (Log *self, GDate *date)
{
        gchar *old_date_str = NULL;
        gchar *new_date_str = NULL;
        gint old_date_len;
        gint new_date_len;

	LOG_IS_VALID(self);
        g_assert (date);
        g_assert (self->prv->model);
        g_assert (g_date_valid (date));

        new_date_str = date_to_string (date); /* convert to utf8 */
        g_assert (new_date_str);

	if (self->prv->date_iter == NULL) {
		self->prv->date_iter = g_new0 (GtkTreeIter,1);
		goto log_write_date_first_run;
	}
	
        gtk_tree_model_get (self->prv->model,
                            self->prv->date_iter,
                            MESSAGE, &old_date_str,
                            -1);
        old_date_len = (gint) g_utf8_strlen (old_date_str, -1);
        new_date_len = (gint) g_utf8_strlen (new_date_str, -1);
        if ((old_date_len != new_date_len)
            || (old_date_len >= new_date_len ?
                g_ascii_strncasecmp (old_date_str, new_date_str, new_date_len)
                : g_ascii_strncasecmp (old_date_str, new_date_str, old_date_len)) != 0) {
                /* new date */
log_write_date_first_run:
                gtk_tree_store_append (GTK_TREE_STORE (self->prv->model),
                                       self->prv->date_iter,
                                       NULL);
                gtk_tree_store_set (GTK_TREE_STORE (self->prv->model),
                                    self->prv->date_iter,
                                    MESSAGE, new_date_str,
                                    DAY_POINTER, date,
                                    -1);
        }
        g_free (new_date_str);
        g_free (old_date_str);
	log_thread_idle (self);
}

void
log_write_lines_range (Log *self, gchar **lines, gint start, gint end)
{
	gint i, n;
	g_assert (end > start);
	n = (end-start)/WRITE_LINES;
	for (i = 0; i < n; i ++) {
		log_write_lines (self,
				 lines + start + i*WRITE_LINES,
				 WRITE_LINES);
	}
	log_write_lines (self, lines + start + n*WRITE_LINES,
			 (end-start)%WRITE_LINES);
}

void
log_write_lines (Log *self, gchar **lines, gint num)
{
	gint i;
	for (i = 0; i < num; i ++)
		log_write_line (self, lines[i]);
	log_thread_idle (self);
}

void
log_write_line (Log *self, gchar *line)
{
	LOG_IS_VALID(self);
	g_assert (line);
	g_assert (self->prv->model);
	g_assert (self->prv->line_iter);

	gtk_tree_store_append (GTK_TREE_STORE (self->prv->model),
				self->prv->line_iter,
				self->prv->date_iter);
	if (!self->prv->update_status) {
		/* normal */
		gtk_tree_store_set (GTK_TREE_STORE (self->prv->model),
				    self->prv->line_iter,
				    MESSAGE, line,
				    DAY_POINTER, NULL,
				    -1);
	} else {
		/* bold */
	        gtk_tree_store_set (GTK_TREE_STORE (self->prv->model),
				    self->prv->line_iter,
	                            MESSAGE, line, 
	                            DAY_POINTER, NULL,
	                            LOG_LINE_WEIGHT, PANGO_WEIGHT_BOLD,
	                            LOG_LINE_WEIGHT_SET, TRUE,
				    -1);
		/* Remember the first bold lines in the model to unset them later */
		if (self->prv->first_bold_row == NULL)
			self->prv->first_bold_row = gtk_tree_model_get_path (self->prv->model, self->prv->line_iter);

	}
	log_thread_idle (self);
}

static gboolean
log_unbold_rows (Log *self)
{
	LogviewWindow *logview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	LOG_IS_VALID(self);

	logview = LOGVIEW_WINDOW (self->prv->window);
	if (logview->curlog != self)
		return TRUE;

	g_assert (self->prv->first_bold_row && self->prv->last_bold_row);

	for (path = self->prv->first_bold_row; 
	     gtk_tree_path_compare (path, self->prv->last_bold_row)<=0;
	     gtk_tree_path_next (path)) {
		gtk_tree_model_get_iter (self->prv->model, &iter, path);
		gtk_tree_store_set (GTK_TREE_STORE (self->prv->model), &iter,
				    LOG_LINE_WEIGHT, PANGO_WEIGHT_NORMAL,
				    LOG_LINE_WEIGHT_SET, TRUE,
				    -1);
		log_thread_idle (self);
	}
  
	gtk_tree_path_free (self->prv->first_bold_row);
	gtk_tree_path_free (self->prv->last_bold_row);
	self->prv->first_bold_row = NULL;
	self->prv->last_bold_row = NULL;
	return FALSE;
}

void log_lock (Log *self)
{
	DATA_LOCK (self);
}

void log_unlock (Log *self)
{
	DATA_UNLOCK (self);
}

