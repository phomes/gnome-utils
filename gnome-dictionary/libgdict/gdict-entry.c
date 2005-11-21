/* gdict-entry.h - GtkEntry widget with dictionary completion
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

#include <gtk/gtkentry.h>
#include <gtk/gtkentrycompletion.h>
#include <gtk/gtkliststore.h>

#include <glib/gi18n.h>

#include "gdict-entry.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"
#include "gdict-utils.h"

#define GDICT_ENTRY_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_ENTRY, GdictEntryPrivate))

struct _GdictEntryPrivate
{
  GdictContext *context;

  guint lookup_start_id;  
  guint match_id;
  guint error_id;
  guint lookup_end_id;
  
  GtkEntryCompletion *completion;

  gchar *word;
  
  GList *results;
};

enum
{
  PROP_0,
  
  PROP_CONTEXT
};

G_DEFINE_TYPE (GdictEntry, gdict_entry, GTK_TYPE_ENTRY);

static void gdict_entry_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);
static void gdict_entry_get_property (GObject      *object,
				      guint         prop_id,
				      GValue       *value,
				      GParamSpec   *pspec);
static void gdict_entry_finalize     (GObject      *object);

static void gdict_entry_lookup       (GdictEntry   *entry);
static void gdict_entry_changed      (GtkEditable  *editable,
				      gpointer      user_data);

static void
gdict_entry_class_init (GdictEntryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkEntryClass *entry_class = GTK_ENTRY_CLASS (klass);
  
  gobject_class->set_property = gdict_entry_set_property;
  gobject_class->get_property = gdict_entry_get_property;
  gobject_class->finalize = gdict_entry_finalize;
  
  /**
   * GdictEntry:context
   *
   * The context object bound to this entry.
   *
   * Since: 0.1
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			_("Context"),
  				   			_("The GdictContext object bound to this entry"),
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  
  g_type_class_add_private (gobject_class, sizeof (GdictEntryPrivate));
}

static void
gdict_entry_init (GdictEntry *entry)
{
  GdictEntryPrivate *priv = GDICT_ENTRY_GET_PRIVATE (entry);
  GtkListStore *store;
  
  entry->priv = priv;
  
  priv->context = NULL;
  priv->results = NULL;
  priv->word = NULL;
  
  priv->completion = gtk_entry_completion_new ();
  
  store = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_entry_completion_set_model (priv->completion, GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));
  
  gtk_entry_completion_set_text_column (priv->completion, 0);
  gtk_entry_completion_set_popup_completion (priv->completion, TRUE);
  gtk_entry_completion_set_minimum_key_length (priv->completion, 5);
  gtk_entry_completion_set_inline_completion (priv->completion, TRUE);
  
  gtk_entry_set_completion (GTK_ENTRY (entry), priv->completion);

  g_signal_connect (entry, "changed", G_CALLBACK (gdict_entry_changed), NULL);
}

static void
gdict_entry_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  GdictEntry *entry = GDICT_ENTRY (object);
  GdictEntryPrivate *priv = entry->priv;
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      if (priv->context)
        g_object_unref (G_OBJECT (priv->context));
      priv->context = g_value_get_object (value);
      g_object_ref (priv->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_entry_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  GdictEntry *entry = GDICT_ENTRY (object);
  GdictEntryPrivate *priv = entry->priv;
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_entry_finalize (GObject *object)
{
  GdictEntry *entry = GDICT_ENTRY (object);
  GdictEntryPrivate *priv = entry->priv;
  
  if (priv->word)
    g_free (priv->word);
  
  if (priv->match_id)
    {
      g_signal_handler_disconnect (priv->context, priv->match_id);
      priv->match_id = 0;
    }
  
  if (priv->context)
    g_object_unref (G_OBJECT (priv->context));
  
  if (priv->results)
    {
      g_list_foreach (priv->results,
      		      (GFunc) gdict_match_unref,
      		      NULL);
      g_list_free (priv->results);
      
      priv->results = NULL;
    }
  
  G_OBJECT_CLASS (gdict_entry_parent_class)->finalize (object);
}

/**
 * gdict_entry_new:
 * @context: a #GdictContext
 * 
 * Creates a new #GdictEntry widget.  This widget is a simple entry
 * with automatic completion bound to a #GdictContext.  When the user
 * begins typing a word, the #GtkEntryCompletion is filled with the
 * matching words retrieved by the context.
 *
 * You might change the context used by the widget by using the
 * gdict_entry_set_context() function.
 *
 * Return value: the newly created #GdictEntry widget.
 */
GtkWidget *
gdict_entry_new (GdictContext *context)
{
  GtkWidget *retval;
  
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  retval = g_object_new (GDICT_TYPE_ENTRY,
  			 "context", context,
  			 NULL);
  
  return retval;
}

/**
 * gdict_entry_set_context:
 * @entry: a #GdictEntry
 * @context: a #GdictContext
 *
 * Sets @context as the #GdictContext used by the entry.
 *
 * This function increases the reference count of @context.
 */
void
gdict_entry_set_context (GdictEntry   *entry,
			 GdictContext *context)
{
  g_return_if_fail (GDICT_IS_ENTRY (entry));
  g_return_if_fail (GDICT_IS_CONTEXT (context));
  
  g_object_set (G_OBJECT (entry), "context", context, NULL);
}

/**
 * gdict_entry_get_context:
 * @entry: a #GdictEntry
 *
 * Retrieves the #GdictContext used by @entry.
 *
 * Return value: the context used by the entry
 */
GdictContext *
gdict_entry_get_context (GdictEntry *entry)
{
  g_return_val_if_fail (GDICT_IS_ENTRY (entry), NULL);
  
  return entry->priv->context;
}

static void
lookup_start_cb (GdictContext *context,
	         GdictEntry   *entry)
{
  GdictEntryPrivate *priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GError *match_error;
  
  g_assert (GDICT_IS_CONTEXT (context));
  g_assert (GDICT_IS_ENTRY (entry));

  priv = entry->priv;
 
  model = gtk_entry_completion_get_model (priv->completion);
  gtk_list_store_clear (GTK_LIST_STORE (model));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, _("Loading matches..."), -1);
  
  priv = entry->priv;
  
  if (priv->results)
    {
      g_list_foreach (priv->results,
      		      (GFunc) gdict_match_unref,
      		      NULL);
      g_list_free (priv->results);
      
      priv->results = NULL;
    }
}

static void
match_found_cb (GdictContext *context,
		GdictMatch   *match,
		GdictEntry   *entry)
{
  GdictEntryPrivate *priv = entry->priv;

  g_message ("(in %s) MATCH: '%s'\n", G_STRFUNC, gdict_match_get_word (match));
  
  /* we must increase the reference count of the match object, since it
   * will be unreferenced as soon as the callback is finished.
   */
  priv->results = g_list_prepend (priv->results, gdict_match_ref (match));
}

static void
lookup_end_cb (GdictContext *context,
	       GdictEntry   *entry)
{
  GdictEntryPrivate *priv = entry->priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *l;

  model = gtk_entry_completion_get_model (priv->completion);
  gtk_list_store_clear (GTK_LIST_STORE (model));
  
  /* no match found */
  if (!priv->results)
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, _("No match found"), -1);
      
      return;
    }
  
  /* walk the list in _reverse_ order */
  for (l = g_list_last (priv->results); l; l = l->prev)
    {
      GdictMatch *match = (GdictMatch *) l->data;
      
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, g_strdup (match->word), -1);
    }
  
  /* destroy the results list */
  g_list_foreach (priv->results,
                  (GFunc) gdict_match_unref,
                  NULL);
  g_list_free (priv->results);
  priv->results = NULL;

  g_free (priv->word);

  /* and finally complete */
  gtk_entry_completion_complete (priv->completion);
}

static void
error_cb (GdictContext *context,
	  GError       *error,
	  GdictEntry   *entry)
{
  g_warning ("Error: %s\n", error->message);
}

static void
gdict_entry_changed (GtkEditable *editable,
		     gpointer     user_data)
{
  GdictEntry *entry = GDICT_ENTRY (editable);

  if (!gtk_entry_get_text (GTK_ENTRY (entry)))
    return;

  gdict_entry_lookup (entry);
}

/* looks up the content of the entry using the context */
static void
gdict_entry_lookup (GdictEntry *entry)
{
  GdictEntryPrivate *priv;
  const gchar *text;
  GError *match_error;
  
  g_assert (GDICT_IS_ENTRY (entry));
  
  priv = entry->priv;
  g_assert (priv->context);
  
  text = gtk_entry_get_text (GTK_ENTRY (entry));
  if (!text)
    return;
  
  if (g_str_has_prefix (text, priv->word))
    {
      g_free (priv->word);
      priv->word = g_strdup (text);

      return;
    }
  
  priv->word = g_strdup (text);
  
  g_signal_connect (priv->context, "lookup-start",
    		    G_CALLBACK (lookup_start_cb), entry);
  g_signal_connect (priv->context, "lookup-end",
  		    G_CALLBACK (lookup_end_cb), entry);
  g_signal_connect (priv->context, "match-found",
  		    G_CALLBACK (match_found_cb), entry);
   
  g_signal_connect (priv->context, "error", G_CALLBACK (error_cb), entry);

  match_error = NULL;
  gdict_context_match_word (priv->context,
		            GDICT_DEFAULT_DATABASE,
		            GDICT_DEFAULT_STRATEGY,
		            priv->word,
		            &match_error);
  if (match_error)
    {
      g_warning ("Unable to match: %s\n", match_error->message);
      g_error_free (match_error);

      return;
    }
}
