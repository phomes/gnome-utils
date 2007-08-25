/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_IFACE_VIEW_H__
#define __LOGVIEW_IFACE_VIEW_H__

#include <glib-object.h>
#include "logview-log.h"

G_BEGIN_DECLS

#define LOGVIEW_TYPE_IFACE_VIEW \
	(logview_iface_view_get_type ())
#define LOGVIEW_IFACE_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), LOGVIEW_TYPE_IFACE_VIEW, \
				     LogviewIView))
#define LOGVIEW_IS_IFACE_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), LOGVIEW_TYPE_IFACE_VIEW))
#define LOGVIEW_GET_VIEW_INTERFACE(obj) \
	(G_TYPE_INSTANCE_GET_INTERFACE((obj), LOGVIEW_TYPE_IFACE_VIEW, \
					LogviewIFaceView))

typedef struct _LogviewIView LogviewIView;
typedef struct _LogviewIFaceView LogviewIFaceView;

struct _LogviewIFaceView {
	GTypeInterface parent;

	/* 
	   Create a list, each element is a group entry,
	   each group entry has a log list, do not free mass.
	   returns NULL if failed.
	*/
	GSList* (*group_lines) (LogviewIView *self, const gchar **lines);

	/* 
	   If possible, convert a string to utf8, free the original
	   string, return the new one.
	*/
	gchar* (*to_utf8) (LogviewIView *self, gchar *str, gssize size);
};

GType logview_iface_view_get_type (void);

/* for logview application internal usage */
GSList* logi_group_lines (LogviewIView *self, const gchar **lines);
gchar* logi_to_utf8 (LogviewIView *self, gchar *str, gssize size);

G_END_DECLS

#endif /* __LOGVIEW_IFACE_VIEW_H__ */

