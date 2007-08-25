#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "misc.h"
#include "logview-debug.h"

#define LOGVIEW_USER_PATH_SUFFIX LOGVIEW_HOME_SUFFIX "/gnome-system-log.log"
static LogviewLevelFlags debug = LOGVIEW_INFO;
static FILE *logf;

void
lv_debug (LogviewLevelFlags level,
	  const gchar *file,
	  gint line,
	  const gchar *function)
{
        g_return_if_fail (file && function && line >= 0);
	if (debug & level) {
		g_fprintf (logf, "\"%s\" %s:%d\n", file, function, line);
		fflush (stdout);
	}
}

void
lv_debug_message (LogviewLevelFlags level,
		  const gchar *file,
		  gint line,
		  const gchar *function,
		  const gchar *format, ...)
{
	if (debug & level) {
		va_list ap;
		g_return_if_fail (file && function && line >= 0);

		if (debug & LOGVIEW_VERBOSE) {
			g_fprintf (logf, "\"%s\" %s:%d ", file, function, line);
			fflush (stdout);
		}

		va_start (ap, format);
		g_vfprintf (logf, format, ap);
		g_fprintf (logf, "\n");
		va_end (ap);
		fflush (stdout);
	}
}

void
logview_debug_init ()
{
	gchar *f = g_build_path (G_DIR_SEPARATOR_S, (gchar*)g_get_home_dir (),
			     LOGVIEW_USER_PATH_SUFFIX, NULL);
	logf = g_fopen (f, "wb");
	g_free (f);

	if (g_getenv ("LOGVIEW_DEBUG") != NULL)	{
		debug |= LOGVIEW_ERR | LOGVIEW_VERBOSE;
	}
	if (g_getenv ("LOGVIEW_VERBOSE") != NULL) {
		debug |= LOGVIEW_VERBOSE;
	}
}

void
logview_debug_destroy ()
{
	fclose (logf);
	logf = NULL;
}

