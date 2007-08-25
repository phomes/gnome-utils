/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#include "logview-iface.h"
#include "logview-iface-view.h"
#include "logview-plugin-manager.h"
#include "logview-debug.h"

static void
logview_iface_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	LV_MARK;
	if (!initialized) {
		initialized = TRUE;
	}
}

static void
logview_iface_class_init (gpointer g_class, gpointer g_class_data)
{
	LogviewIFaceView *self = (LogviewIFaceView *)g_class;
	LV_MARK;
	self->group_lines = NULL;
	self->to_utf8 = NULL;
}

GType
logview_iface_view_get_type (void)
{
	static GType type = 0;
	LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewIFaceView),
			logview_iface_base_init,   /* base_init */
			NULL,   /* base_finalize */
			logview_iface_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE,
					       g_intern_static_string ("LogviewIFaceViewType"),
					       &info, 0);
		PluginInterface_register (PF_LOG_VIEW, type);
		g_type_interface_add_prerequisite (type, LOGVIEW_TYPE_IFACE);
	}
	return type;
}

/**
 * logi_group_lines:
 * @self: a #LogviewIView instance.
 * @lines: an array of strings, terminates in NULL.
 * 
 * Passes an array of strings to logi_group_lines(),
 * the plugin should try to group these strings by date.
 * 
 * Returns: a single list with each element is a #Day object.
 **/
GSList*
logi_group_lines (LogviewIView *self, const gchar **lines)
{
	LV_MARK;
	return LOGVIEW_GET_VIEW_INTERFACE (self)->group_lines(self, lines);
}

/**
 * logi_to_utf8:
 * @self: a #LogviewIView instance.
 * @str: a string.
 * @size: the length of @str.
 * 
 * Gnome System Log will call this routine before displays the string on the screen.
 * 
 * Returns: a converted string, which will be freed by Gnome System Log.
 **/
gchar*
logi_to_utf8 (LogviewIView *self, gchar *str, gssize size)
{
	LogviewIFaceView *iface = LOGVIEW_GET_VIEW_INTERFACE (self);
	LV_MARK;
	if (iface->to_utf8)
		return iface->to_utf8 (self, str, size);
	return str;
}
