/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

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

#ifndef __LOGVIEW_LOG_H__
#define __LOGVIEW_LOG_H__

#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "logview-plugin.h"
#include "logview-iface.h"

G_BEGIN_DECLS

#define LOGVIEW_TYPE_LOG		(log_get_type ())
#define LOGVIEW_LOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), LOGVIEW_TYPE_LOG, Log))
#define LOGVIEW_LOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), LOGVIEW_TYPE_LOG, LogClass))
#define LOGVIEW_IS_LOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), LOGVIEW_TYPE_LOG))
#define LOGVIEW_IS_LOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), LOGVIEW_TYPE_LOG))
#define LOGVIEW_LOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), LOGVIEW_TYPE_LOG, LogClass))

typedef struct _Log Log;
typedef struct _LogClass LogClass;
typedef struct _LogPrivate LogPrivate;

struct _Log
{
	GObject parent;
	/*< private >*/
	LogPrivate *prv;
};

struct _LogClass {
	GObjectClass parent;
};

/**
 * LogProtocol:
 * @UNKNOWN_LOG: represents the log can't be recognized.
 * But still has the opportunity to be handled by other plugins.
 * @BAD_LOG: represents the log is not a log or a bad log.
 * If in one time a log is marked with this, it's permanently 
 * ignored and removed from the log name list.
 * @PLAIN_LOG: a ASCII log.
 * @GZIP_LOG: a zipped or gzipped log.
 * @PIPE_LOG: a log can be open by other command.
 * @USER_DEFINED_LOG: a plugin can define a log protocol for itself, 
 * e.g. %USER_DEFINED_LOG + 1.
 */
typedef enum {
	UNKNOWN_LOG=0,
	BAD_LOG,
	PLAIN_LOG,
	GZIP_LOG,
	PIPE_LOG,
	USER_DEFINED_LOG
} LogProtocol;

typedef enum {
	TH_EVENT_EMPTY		= 0,
	TH_EVENT_INIT_READ	= 1<<1,
	TH_EVENT_UPDATE		= 1<<2,
	TH_EVENT_EXIT		= 1<<8
} ThreadEvent;

/**
 * Day:
 * @date: a date, without time value.
 * @first_line: First line for this day in the log.
 * @last_line: Last line for this day in the log.
 * @expand: Collapse all the logs in this @date if %FALSE, else expand all.
 * @path: Gtk tree path of the day item display in main view of the log.
 * 
 * A structure used for grouping and recording the logs by date.
 */
typedef struct _Day
{
    GDate *date;
    long first_line, last_line;
    gboolean expand;
    GtkTreePath *path;
} Day;

#define OLD_LOG_NUM	5

/**
 * log_io:
 * @log: a #Log instance.
 * 
 * Returns: the %PF_LOG_IO type plugin.
 */
#define log_io(log)		(LOGVIEW_IFACE_IO (log->prv->ext_data [PF_LOG_IO]))

/**
 * log_view:
 * @log: a #Log instance.
 * 
 * Returns: the %PF_LOG_VIEW type plugin.
 */
#define log_view(log)		(LOGVIEW_IFACE_VIEW (log->prv->ext_data [PF_LOG_VIEW]))

/**
 * log_name:
 * @self: a #Log instance.
 * 
 * Get the log name.
 * 
 * Returns: a constant string which should not be free.
 */

/**
 * log_get_protocol:
 * @self: a #Log instance.
 * 
 * Get the #LogProtocol value of self's.
 * 
 * Returns: the #LogProtocol value.
 */

/**
 * log_set_protocol:
 * @self: a #Log instance.
 * @protocol: a #LogProtocol value.
 * 
 * Set the #LogProtocol value of self's.
 */

/**
 * is_log:
 * @log: a #Log instance.
 * 
 * Determining whether @log is a #Log instance.
 * 
 * Returns: TRUE if it is.
 */


/**
 * log_set_child:
 * @self: a #Log instance.
 * @index: the child log id. it should be no less than zero and less #OLD_LOG_NUM
 * @child: a #Log instance.
 * 
 * Access child log instances.
 */
void log_set_child (Log *self, gint index, Log *child);

/**
 * log_get_child:
 * @self: a #Log instance.
 * @index: the child log id. it should be no less than zero and less #OLD_LOG_NUM
 * 
 * Access child log instances.
 * 
 * Returns: the child #Log instance.
 */
Log* log_get_child (Log *self, gint index);

/**
 * log_test_func:
 * @self: a #Log instance.
 * @pf: a #PluginFunc index.
 * 
 * Returns: if a #pf type plugin is assigned to #self, returns TRUE.
 */
gboolean log_test_func (Log *self, PluginFunc pf);

/**
 * log_set_func:
 * @self: a #Log instance.
 * @pf: a #PluginFunc index.
 * @plugin: a #LogviewPlugin instance.
 * 
 * Assign a #pf type plugin to #self.
 */
void log_set_func (Log *self, PluginFunc pf, LogviewPlugin *plugin);

void log_notify (Log *self, ThreadEvent te);
gchar *log_read (Log *self);
void log_write_line (Log *self, gchar *line);
void log_write_lines (Log *self, gchar **lines, gint num);
void log_write_lines_range (Log *self, gchar **lines, gint start, gint end);
void log_write_date (Log *self, GDate *date);
void log_write_day (Log *self, Day *day);

extern GType log_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __LOGVIEW_LOG_H__ */
