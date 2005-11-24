/* gdict-defbox.h - display widget for dictionary definitions
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

#ifndef __GDICT_DEFBOX_H__
#define __GDICT_DEFBOX_H__

#include <gtk/gtkvbox.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_DEFBOX		(gdict_defbox_get_type ())
#define GDICT_DEFBOX(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_DEFBOX, GdictDefbox))
#define GDICT_IS_DEFBOX(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_DEFBOX))
#define GDICT_DEFBOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_DEFBOX, GdictDefboxClass))
#define GDICT_IS_DEFBOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_DEFBOX))
#define GDICT_DEFBOX_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_DEFBOX, GdictDefboxClass))

typedef struct _GdictDefbox        GdictDefbox;
typedef struct _GdictDefboxClass   GdictDefboxClass;
typedef struct _GdictDefboxPrivate GdictDefboxPrivate;

struct _GdictDefbox
{
  /*< private >*/
  GtkVBox parent_instance;
  
  GdictDefboxPrivate *priv;
};

struct _GdictDefboxClass
{
  GtkVBoxClass parent_class;
  
  /* padding for future expansion */
  void (*_gdict_defbox_1) (void);
  void (*_gdict_defbox_2) (void);
  void (*_gdict_defbox_3) (void);
  void (*_gdict_defbox_4) (void);
};

GType         gdict_defbox_get_type    (void) G_GNUC_CONST;

GtkWidget *   gdict_defbox_new         (GdictContext *context);
void          gdict_defbox_set_context (GdictDefbox  *defbox,
					GdictContext *context);
GdictContext *gdict_defbox_get_context (GdictDefbox  *defbox);

void          gdict_defbox_clear       (GdictDefbox  *defbox);
void          gdict_defbox_lookup      (GdictDefbox  *defbox,
					const gchar  *word);

G_END_DECLS

#endif /* __GDICT_DEFBOX_H__ */
