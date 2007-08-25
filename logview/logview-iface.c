/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

 
#include "logview-iface.h"
#include "logview-debug.h"

static void
logview_iface_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	//LV_MARK;
	if (!initialized) {
		initialized = TRUE;
	}
}

static void
logview_iface_class_init (gpointer g_class, gpointer g_class_data)
{
	LogviewInterface *self = (LogviewInterface *)g_class;
	//LV_MARK;
	self->can_handle = NULL;
}

GType
logview_iface_get_type (void)
{
	static GType type = 0;
	//LV_MARK;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewInterface),
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
					       g_intern_static_string ("LogviewInterfaceType"),
					       &info, 0);
	}
	return type;
}

/**
 * logi_can_handle:
 * @self: a #LogviewIFace instance.
 * @log: a #Log instance.
 * 
 * Check whether the @log can be handled.
 * 
 * Returns: If this interface is not overwritten,
 * it default returns TRUE means the @log can be recognized.
 **/
gboolean
logi_can_handle (LogviewIFace *self, struct _Log* log)
{
	LogviewInterface *iface = LOGVIEW_GET_INTERFACE (self);
	//LV_MARK;
	if (iface->can_handle)
		return (iface)->can_handle(self, log);
	return TRUE;
}

