/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#ifndef __LOGVIEW_IFACE_IO_H__
#define __LOGVIEW_IFACE_IO_H__

#include <sys/types.h>
#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_IFACE_IO \
	(logview_iface_io_get_type ())
#define LOGVIEW_IFACE_IO(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_IFACE_IO, \
				     LogviewIIO))
#define LOGVIEW_IS_IFACE_IO(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_IFACE_IO))
#define LOGVIEW_GET_IO_INTERFACE(obj) \
	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), LOGVIEW_TYPE_IFACE_IO, \
					LogviewIFaceIO))

typedef struct _LogviewIIO LogviewIIO; /* dummy object */
typedef struct _LogviewIFaceIO LogviewIFaceIO;

struct _LogviewIFaceIO {
	GTypeInterface parent;

	gboolean (*can_monitor) (LogviewIIO *self);
	void (*extract_filepath) (LogviewIIO *self,
				  gchar** dirname, gchar** filename);
	gboolean (*has_updated) (LogviewIIO *self);
	void (*update) (LogviewIIO *self);
	size_t (*read) (LogviewIIO *self, void* buffer, size_t size);
	off_t (*seek) (LogviewIIO *self, off_t offset, int whence);
	off_t (*tell) (LogviewIIO *self);
	off_t (*get_size) (LogviewIIO* self);
	time_t (*get_modified_time) (LogviewIIO* self);

};

extern GType logview_iface_io_get_type (void) G_GNUC_CONST;

/* for logview application internal usage */
extern gboolean logi_can_monitor (LogviewIIO *self);
extern void logi_extract_filepath (LogviewIIO *self,
			    gchar** dirname, gchar** filename);
extern gboolean logi_has_updated (LogviewIIO *self);
extern void logi_update (LogviewIIO *self);
extern size_t logi_read (LogviewIIO *self, void* buffer, size_t size);
extern off_t logi_seek (LogviewIIO *self, off_t offset, int whence);
extern off_t logi_tell (LogviewIIO *self);
extern off_t logi_get_size (LogviewIIO* self);
extern time_t logi_get_modified_time (LogviewIIO* self);

G_END_DECLS

#endif /* __LOGVIEW_IFACE_H__ */
