/* gdict-defbox.c - display widget for dictionary definitions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gdict-defbox.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"


#define GDICT_DEFBOX_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DEFBOX, GdictDefbox))

struct _GdictDefboxPrivate
{
  GtkWidget *textview;
  
  GtkWidget *find_box;
  GtkWidget *find_entry;
  GtkWidget *find_next;
  GtkWidget *find_previous;
  GtkWidget *find_label;
  GtkWidget *find_close;

  GdictContext *context;
  
  gchar *word;
  gchar *database;
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_WORD,
  PROP_DATABASE
};



G_DEFINE_TYPE (GdictDefbox, gdict_defbox, GTK_TYPE_VBOX);



static void
gdict_defbox_finalize (GObject *object)
{

}

static void
gdict_defbox_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{

}

static void
gdict_defbox_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{

}

static GObject *
gdict_defbox_default_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_params)
{
  GdictDefbox *defbox;
  GObject *object;
  
  object = G_OBJECT_CLASS (gdict_defbox_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  defbox = GDICT_DEFBOX (object);
  
  g_assert (defbox->priv->context);
  
  gtk_widget_push_composite_child ();
  
  
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_defbox_class_init (GdictDefboxClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->constructor = gdict_defbox_default_constructor;
  gobject_class->set_property = gdict_defbox_set_property;
  gobject_class->get_property = gdict_defbox_get_property;
  gobject_class->finalize = gdict_defbox_finalize;
  
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("emphasis-color",
  					   		       _("Emphasys Color"),
  					   		       _("Color of emphasised text"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  gtk_widget_class_install_style_property (widget_class,
  					   g_param_spec_boxed ("link-color",
  					   		       _("Link Color"),
  					   		       _("Color of hyperlinks"),
  					   		       GDK_TYPE_COLOR,
  					   		       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_type_class_add_private (klass, sizeof (GdictDefboxPrivate));
}

static void
gdict_defbox_init (GdictDefbox *defbox)
{

}

/**
 * gdict_defbox_new:
 * @context: a #GdictContext
 *
 * Creates a new text definition widget. Use this widget to search for
 * a word using @context, and to show the resulting definition.
 *
 * Return value: a new #GdictDefbox widget.
 */
GtkWidget *
gdict_defbox_new (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_DEFBOX,
                       "context", context,
                       NULL);
}
