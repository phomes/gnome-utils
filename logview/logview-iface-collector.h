/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_IFACE_COLLECTOR_H__
#define __LOGVIEW_IFACE_COLLECTOR_H__

#include <glib-object.h>
#include "logview-log.h"

G_BEGIN_DECLS

#define LOGVIEW_TYPE_IFACE_COLLECTOR \
	(logview_iface_collector_get_type ())
#define LOGVIEW_IFACE_COLLECTOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), LOGVIEW_TYPE_IFACE_COLLECTOR, \
				     LogviewICollector))
#define LOGVIEW_IS_IFACE_COLLECTOR(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), LOGVIEW_TYPE_IFACE_COLLECTOR))
#define LOGVIEW_GET_COLLECTOR_INTERFACE(obj) \
	(G_TYPE_INSTANCE_GET_INTERFACE((obj), LOGVIEW_TYPE_IFACE_COLLECTOR, \
					LogviewIFaceCollector))

typedef struct _LogviewICollector LogviewICollector; /* dummy object */
typedef struct _LogviewIFaceCollector LogviewIFaceCollector;

struct _LogviewIFaceCollector {
	GTypeInterface parent;

	/* Generate a new list of log names, need be freed */
	GSList* (*get_logs) (LogviewICollector *self);

	/* Create a log instance from path name, returns NULL if fails. */
	Log* (*config_log_from_path) (LogviewICollector *self,
				      const gchar *log_name);

	/* configure a log instance */
	gboolean (*config_log) (LogviewICollector *self, Log *log);
};

extern GType logview_iface_collector_get_type (void) G_GNUC_CONST;

/* for logview application internal usage */
extern GSList* logi_get_logs (LogviewICollector *self);
extern Log* logi_config_log_from_path (LogviewICollector *self,
				const gchar *log_name);
extern gboolean logi_config_log (LogviewICollector *self, Log *log);

G_END_DECLS

#endif /* __LOGVIEW_IFACE_COLLECTOR_H__ */
