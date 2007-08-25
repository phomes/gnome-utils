/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */
 
#include <glib.h>
#include <stdio.h>
#include <signal.h>
#include <glib.h>
#include "logview-plugin.h"
#include "logview-iface-view.h"
#include "logview-debug.h"
#include "logview-log.h"
#include "misc.h"


#define PLUGIN_BASEVIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), PLUGIN_TYPE_BASEVIEW, Baseview))
#define PLUGIN_BASEVIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), PLUGIN_TYPE_BASEVIEW, BaseviewClass))
#define BASEVIEW_IS_PLUGIN(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLUGIN_TYPE_BASEVIEW))
#define BASEVIEW_IS_PLUGIN_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), PLUGIN_TYPE_BASEVIEW))
#define PLUGIN_BASEVIEW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), PLUGIN_TYPE_BASEVIEW, BaseviewClass))

#undef PLUGIN_PRIO
#define PLUGIN_PRIO	PLUG_PRIO_BASE
#undef PLUGIN_DESC
#define PLUGIN_DESC	"Group by date and try to convert to UTF-8"

typedef struct _Baseview Baseview;
typedef struct _BaseviewClass BaseviewClass;

struct _Baseview {
	LogviewPlugin parent;
};

struct _BaseviewClass {
	LogviewPluginClass parent;
};


/* class implement */
static void iface_init (gpointer g_iface, gpointer iface_data);
static void view_init (gpointer g_iface, gpointer iface_data);

static void
baseview_class_init (gpointer g_class, gpointer g_class_data)
{
	LV_MARK;
}

static void
baseview_instance_init (GTypeInstance *instance, gpointer g_class_data)
{
	LV_MARK;
}

static GType
baseview_get_type (GTypeModule* module)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (BaseviewClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			baseview_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (Baseview),
			0,      /* n_preallocs */
			baseview_instance_init    /* instance_init */
		};
		static const GInterfaceInfo iface_info = {
			iface_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		static const GInterfaceInfo view_info = {
			view_init,	/* interface_init */
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		type = g_type_module_register_type (module,
						    LOGVIEW_TYPE_PLUGIN,
						    "BaseviewType",
						    &info,
						    0);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE,
			&iface_info);
		g_type_module_add_interface (module, type, LOGVIEW_TYPE_IFACE_VIEW,
			&view_info);
	}
	return type;
}

/* interfaces */
static gboolean
can_handle(Baseview *self, Log *log)
{
	LV_MARK;
	return TRUE;
}

static gint 
days_compare (gconstpointer a, gconstpointer b)
{
	const Day *day1 = a, *day2 = b;
	return (g_date_compare (day1->date, day2->date));
}

/* log_read_dates
   Read all dates which have a log entry to create calendar.
   All dates are given with respect to the 1/1/1970
   and are then corrected to the correct year once we
   reach the end.
*/

static GSList *
baseview_read_dates (LogviewIView *self, gchar **buffer_lines, time_t current)
{
	int offsetyear = 0, current_year;
	GSList *days = NULL, *days_copy;
	GDate *date, *newdate;
	struct tm *tmptm;
	gchar *date_string;
	Day *day;
	gboolean done = FALSE;
	int i, n, rangemin, rangemax;

	if (buffer_lines == NULL)
		return NULL;

	n = g_strv_length (buffer_lines);
	for (i=0; buffer_lines[i] == NULL && i < n; i++);
	if (i == n)
		return NULL;

	tmptm = localtime (&current);
	current_year = tmptm->tm_year + 1900;

	/* Start building the list */
	/* Scanning each line to see if the date changed is too slow,
	   so we proceed in a recursive fashion */

	date = string_get_date (buffer_lines[i]);
	if ((date==NULL)|| !g_date_valid (date)) {
		return NULL;
	}

	g_date_set_year (date, current_year);
	day = g_new0 (Day, 1);
	days = g_slist_append (days, day);

	day->date = date;
	day->first_line = i;
	day->last_line = -1;
	date_string = get_date_string (buffer_lines[i]);

	rangemin = 0;
	rangemax = n-1;

	while (!done) {
	   
		i = n-1;
		while (day->last_line < 0) {

			if (g_str_has_prefix (buffer_lines[i], date_string)) {
				if (i == (n-1)) {
					day->last_line = i;
					done = TRUE;
					break;
				} else {
					if (!g_str_has_prefix (buffer_lines[i+1], date_string)) {
						day->last_line = i;
						break;
					} else {
						rangemin = i;
						i = (int) ( ((float) i + (float) rangemax)/2.);
					}
				}
			} else {
				rangemax = i;
				i = (int) (((float) rangemin + (float) i)/2.);				
			}

		}
	   
		g_free (date_string);

		if (!done) {
			/* We need to find the first line now that has a date
			   Logs can have some messages without dates ... */
			newdate = NULL;
			while (newdate == NULL && !done && i < n) {
				i++;
				date_string = get_date_string (buffer_lines[i]);
				if (date_string == NULL)
					continue;
				g_assert (newdate == NULL);
				newdate = string_get_date (buffer_lines[i]);
		   
				if (newdate == NULL && i==n-1)
					done = TRUE;
			}

			day->last_line = i-1;

			/* Append a day to the list */	
			if (newdate) {
				g_date_set_year (newdate, current_year + offsetyear);	
				if (g_date_compare (newdate, date) < 1) {
					offsetyear++; /* newdate is next year */
					g_date_add_years (newdate, 1);
				}
		   
				date = newdate;
				day = g_new0 (Day, 1);
				days = g_slist_append (days, day);
		   
				day->date = date;
				day->first_line = i;
				day->last_line = -1;
				rangemin = i;
				rangemax = n;
			}
		}
	}

	/* Correct years now. We assume that the last date on the log
	   is the date last accessed */

	for (days_copy = days; days_copy != NULL; days_copy = g_slist_next (days_copy)) {	   
		day = days_copy -> data;
		g_date_subtract_years (day->date, offsetyear);
	}
   
	/* Sort the days in chronological order */
	days = g_slist_sort (days, days_compare);

	return (days);
}

static GSList*
group_lines (LogviewIView *self, const gchar **lines)
{
	LV_MARK;
	return baseview_read_dates (self, (gchar **)lines, time (NULL));
}

static void
iface_init (gpointer g_iface, gpointer iface_data)
{
	LogviewInterface *iface = (LogviewInterface*)g_iface;
	LV_MARK;
	iface->can_handle = (gboolean (*) (LogviewIFace*, Log*)) can_handle;
}

static gchar*
to_utf8 (LogviewIView *self, gchar *str)
{
	char *out;
	if (g_utf8_validate (str, -1, NULL))
		out = str;
	else {
		if ((out = g_locale_to_utf8 (str, -1, NULL, NULL, NULL)) != NULL)
			g_free (str);
		else
			out = str;
	}
	return out;
}

static void
view_init (gpointer g_iface, gpointer iface_data)
{
	LogviewIFaceView *self = (LogviewIFaceView*)g_iface;
	LV_MARK;
	self->group_lines = (GSList* (*) (LogviewIView*, const gchar**)) group_lines;
	self->to_utf8 = (gchar* (*) (LogviewIView*, gchar*, gssize)) to_utf8;
}

LOGVIEW_INIT_PLUGIN (baseview_get_type)

