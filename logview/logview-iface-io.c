/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2006 Lin Ma <Lin.Ma@sun.com>
 */

#include "logview-iface.h"
#include "logview-iface-io.h"
#include "logview-debug.h"

static void
logview_iface_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}

static void
logview_iface_class_init (gpointer g_class, gpointer g_class_data)
{
	LogviewIFaceIO *self = (LogviewIFaceIO *)g_class;
	self->can_monitor = NULL;
	self->extract_filepath = NULL;
	self->has_updated = NULL;
	self->update = NULL;
	self->read = NULL;
	self->seek = NULL;
	self->tell = NULL;
	self->get_size = NULL;
	self->get_modified_time = NULL;
}

GType
logview_iface_io_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			/* You fill this structure. */
			sizeof (LogviewIFaceIO),
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
					       g_intern_static_string ("LogviewIFaceIOType"),
					       &info, 0);
		g_type_interface_add_prerequisite (type, LOGVIEW_TYPE_IFACE);
	}
	return type;
}

/**
 * logi_can_monitor:
 * @self: a #LogviewIIO instance.
 * 
 * Check whether @self can be monitored.
 * 
 * Returns: Return TRUE, if it can.
 **/
gboolean
logi_can_monitor (LogviewIIO *self)
{
	LogviewIFaceIO *iface = LOGVIEW_GET_IO_INTERFACE (self);
	LV_MARK;
	return iface->can_monitor(self);
}

/**
 * logi_has_updated:
 * @self: a #LogviewIIO instance.
 * 
 * Check if @self has changed since last reading happens.
 *
 * Returns: Return TRUE, if it can.
 * */
gboolean
logi_has_updated (LogviewIIO *self)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->has_updated(self);
}

/**
 * logi_has_updated:
 * @self: a #LogviewIIO instance.
 * 
 * Update @self to a new state, do not report True
 * if call logi_has_updated() any more except a new change is made to @self.
 */
void
logi_update (LogviewIIO *self)
{
	LV_MARK;
	LOGVIEW_GET_IO_INTERFACE (self)->update(self);
}

/**
 * logi_read:
 * @self: a #LogviewIIO instance.
 * @buffer: a buffer.
 * @size: the length of the @buffer.
 * 
 * read, seek, tell are similiar to Standard C read, fseek, ftell.
 * whence := SEEK_SET | SEEK_CUR | SEEK_END
 * returns -1 if met error.
 * but no errno currently.
 * 
 * Returns: Upon successful completion, logi_read() returns the size of
 * successfully read. If @size is equal to 0, logi_read() returns 0 and
 * the contents of the @buffer remain unchanged. Otherwise, if a read error
 * occurs, returns -1.
 */
size_t
logi_read (LogviewIIO *self, void* buffer, size_t size)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->read(self, buffer, size);
}

/**
 * logi_seek:
 * @self: a #LogviewIIO instance.
 * @offset: a offset.
 * @whence: like fseek, accept SEEK_SET | SEEK_CUR | SEEK_END.
 * 
 * read, seek, tell are similiar to Standard C read, fseek, ftell.
 * but no errno currently.
 * 
 * Returns: -1 if met error.
 */
off_t
logi_seek (LogviewIIO *self, off_t offset, int whence)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->seek(self, offset, whence);
}

/**
 * logi_tell:
 * @self: a #LogviewIIO instance.
 * 
 * read, seek, tell are similiar to Standard C read, fseek, ftell.
 * but no errno currently.
 * 
 * Returns: On successful completion, returns the current offset
 * opposite to the log head; returns -1 if met error.
 */
off_t
logi_tell (LogviewIIO *self)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->tell(self);
}

/**
 * logi_get_size:
 * @self: a #LogviewIIO instance.
 * 
 * Depends on logi_update(), report the size in one state of @self.
 * 
 * Returns: On successful completion, returns the current size of @self,
 * else returns 0.
 **/
off_t
logi_get_size (LogviewIIO* self)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->get_size(self);
}

/**
 * logi_get_modified_time:
 * @self: a #LogviewIIO instance.
 * 
 * Depends on logi_update(), report the modified time in one state of @self.
 * 
 * Returns: On successful completion, returns the current size of @self,
 * else returns 0.
 **/
time_t
logi_get_modified_time (LogviewIIO* self)
{
	LV_MARK;
	return LOGVIEW_GET_IO_INTERFACE (self)->get_modified_time(self);
}

/**
 * logi_extract_filepath:
 * @self: a #LogviewIIO instance.
 * @dirname: returns a pointer points to the new alloc buffer
 * contains the path name of @self.
 * If passes NULL, the plugin should ignore this argument.
 * @filename: returns a pointer points to the new alloc buffer
 * contains the file name of @self.
 * If passes NULL, the plugin should ignore this argument.
 * 
 * Extract the path name and the file name from $self, which will be displayed on
 * Gnome System Log left tree pane.
 **/
void
logi_extract_filepath (LogviewIIO *self, gchar** dirname, gchar** filename)
{
	LV_MARK;
	LOGVIEW_GET_IO_INTERFACE (self)->extract_filepath(self, dirname, filename);
}
