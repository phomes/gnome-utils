/* gdict-source.h - Source configuration for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __GDICT_SOURCE_H__
#define __GDICT_SOURCE_H__

#include <glib-object.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_SOURCE		    (gdict_source_get_type ())
#define GDICT_SOURCE(obj)		    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SOURCE, GdictSource))
#define GDICT_IS_SOURCE(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SOURCE))

typedef struct _GdictSource        GdictSource;
typedef struct _GdictSourceClass   GdictSourceClass;
typedef struct _GdictSourcePrivate GdictSourcePrivate;

typedef enum
{
  GDICT_SOURCE_TRANSPORT_DICTD,
  
  GDICT_SOURCE_TRANSPORT_INVALID /* only for debug */
} GdictSourceTransport;

#define GDICT_SOURCE_ERROR	(gdict_source_error_quark ())

typedef enum
{
  GDICT_SOURCE_ERROR_PARSE,
  GDICT_SOURCE_ERROR_INVALID_TRANSPORT,
  GDICT_SOURCE_ERROR_INVALID_BAD_PARAMETER
} GdictSourceError;

GQuark gdict_source_error_quark (void);

struct _GdictSource
{
  /*< private >*/
  GObject parent_instance;
  
  GdictSourcePrivate *priv;
};

struct _GdictSourceClass
{
  /*< private >*/
  GObjectClass parent_class;
};

GType gdict_source_get_type (void) G_GNUC_CONST;

GdictSource *         gdict_source_new             (void);
gboolean              gdict_source_load_from_file  (GdictSource           *source,
						    const gchar           *filename,
						    GError               **error);
gboolean              gdict_source_load_from_data  (GdictSource           *source,
						    const gchar           *data,
						    gsize                  length,
						    GError               **error);
gchar *               gdict_source_to_data         (GdictSource           *source,
						    gsize                 *length,
						    GError               **error) G_GNUC_MALLOC;
						    
void                  gdict_source_set_name        (GdictSource           *source,
						    const gchar           *name);
G_CONST_RETURN gchar *gdict_source_get_name        (GdictSource           *source);
void                  gdict_source_set_description (GdictSource           *source,
						    const gchar           *description);
G_CONST_RETURN gchar *gdict_source_get_description (GdictSource           *source);
void                  gdict_source_set_transport   (GdictSource           *source,
						    GdictSourceTransport   transport);
GdictSourceTransport  gdict_source_get_transport   (GdictSource           *source);
GdictContext *        gdict_source_get_context     (GdictSource           *source);
void                  gdict_source_set_context     (GdictSource           *source,
						    GdictContext          *context);

G_END_DECLS

#endif /* __GDICT_SOURCE_H__ */
