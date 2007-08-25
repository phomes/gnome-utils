/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_IFACE_H__
#define __LOGVIEW_IFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_IFACE \
	(logview_iface_get_type ())
#define LOGVIEW_IFACE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), LOGVIEW_TYPE_IFACE, \
				     LogviewIFace))
#define LOGVIEW_IS_IFACE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), LOGVIEW_TYPE_IFACE))
#define LOGVIEW_GET_INTERFACE(obj) \
	(G_TYPE_INSTANCE_GET_INTERFACE((obj), LOGVIEW_TYPE_IFACE, \
					LogviewInterface))

struct _Log;

typedef struct _LogviewIFace LogviewIFace; /* dummy object */
typedef struct _LogviewInterface LogviewInterface;

struct _LogviewInterface {
	GTypeInterface parent;

	gboolean (*can_handle) (LogviewIFace *self, struct _Log* log);
};

GType logview_iface_get_type (void);

/* for logview application internal usage */
gboolean logi_can_handle (LogviewIFace *self, struct _Log* log);

G_END_DECLS

#endif /* __LOGVIEW_IFACE_H__ */

