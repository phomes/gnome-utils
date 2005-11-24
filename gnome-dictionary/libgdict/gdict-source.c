/* gdict-source.c - Source configuration for Gdict
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

#include <glib/gi18n.h>

#include "gdict-source.h"
#include "gdict-client-context.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"

/* Main group */
#define SOURCE_GROUP		"Dictionary Source"

/* Common keys */
#define SOURCE_KEY_NAME		"Name"
#define SOURCE_KEY_DESCRIPTION	"Description"
#define SOURCE_KEY_TRANSPORT	"Transport"

/* dictd transport keys */
#define SOURCE_KEY_HOSTNAME	"Hostname"
#define SOURCE_KEY_PORT		"Port"

#define GDICT_SOURCE_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_SOURCE, GdictSourcePrivate))

struct _GdictSourcePrivate
{
  gchar *filename;
  GKeyFile *keyfile;
  
  gchar *name;
  gchar *description;
  
  GdictSourceTransport transport;
  
  GdictContext *context;
};

enum
{
  PROP_0,
  
  PROP_FILENAME,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_TRANSPORT,
  PROP_CONTEXT
};

/* keep in sync with GdictSourceTransport */
static const gchar *valid_transports[] =
{
  "dictd",	/* GDICT_SOURCE_TRANSPORT_DICTD */
  
  NULL		/* GDICT_SOURCE_TRANSPORT_INVALID */
};

GQuark
gdict_source_error_quark (void)
{
  static GQuark quark = 0;
  
  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gdict-source-error-quark");
  
  return quark;
}


G_DEFINE_TYPE (GdictSource, gdict_source, G_TYPE_OBJECT);



static void
gdict_source_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictSource *source = GDICT_SOURCE (object);
  
  switch (prop_id)
    {
    case PROP_NAME:
      gdict_source_set_name (source, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      gdict_source_set_description (source, g_value_get_string (value));
      break;
    case PROP_TRANSPORT:
      gdict_source_set_transport (source, g_value_get_enum (value));
      break;
    case PROP_CONTEXT:
      gdict_source_set_context (source, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictSourcePrivate *priv = GDICT_SOURCE_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, priv->filename);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, priv->description);
      break;
    case PROP_TRANSPORT:
      g_value_set_enum (value, priv->transport);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_finalize (GObject *object)
{
  GdictSourcePrivate *priv = GDICT_SOURCE_GET_PRIVATE (object);
  
  g_free (priv->filename);
  
  if (priv->keyfile)
    g_key_file_free (priv->keyfile);
  
  g_free (priv->name);
  g_free (priv->description);
  
  if (priv->context)
    g_object_unref (priv->context);
  
  G_OBJECT_CLASS (gdict_source_parent_class)->finalize (object);
}

static void
gdict_source_class_init (GdictSourceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_source_set_property;
  gobject_class->get_property = gdict_source_get_property;
  gobject_class->finalize = gdict_source_finalize;
  
  /**
   * GdictSource:filename
   *
   * The filename used by this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_FILENAME,
  				   g_param_spec_string ("filename",
  				   			_("Filename"),
  				   			_("The filename used by this dictionary source"),
  				   			NULL,
  				   			G_PARAM_READABLE));
  /**
   * GdictSource:name
   *
   * The display name of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_NAME,
  				   g_param_spec_string ("name",
  				   			_("Name"),
  				   			_("The display name of this dictonary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:description
   *
   * The description of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_DESCRIPTION,
  				   g_param_spec_string ("description",
  				   			_("Description"),
  				   			_("The description of this dictionary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:transport
   *
   * The transport mechanism used by this source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_TRANSPORT,
  				   g_param_spec_enum ("transport",
  				   		      _("Transport"),
  				   		      _("The transport mechanism used by this dictionary source"),
  				   		      GDICT_TYPE_SOURCE_TRANSPORT,
  				   		      GDICT_SOURCE_TRANSPORT_INVALID,
  				   		      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:context
   *
   * The #GdictContext bound to this source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			_("Context"),
  				   			_("The GdictContext bound to this source"),
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  
  g_type_class_add_private (klass, sizeof (GdictSourcePrivate));
}

static void
gdict_source_init (GdictSource *source)
{
  GdictSourcePrivate *priv;
  
  priv = GDICT_SOURCE_GET_PRIVATE (source);
  
  priv->filename = NULL;
  priv->keyfile = g_key_file_new ();
  
  priv->name = NULL;
  priv->description = NULL;
  priv->transport = GDICT_SOURCE_TRANSPORT_INVALID;
  
  priv->context = NULL;
}

/**
 * gdict_source_new:
 *
 * Creates an empty #GdictSource object.  Use gdict_load_from_file() to
 * read an existing dictionary source definition file.
 *
 * Return value: an empty #GdictSource
 */
GdictSource *
gdict_source_new (void)
{
  return g_object_new (GDICT_TYPE_SOURCE, NULL);
}

static GdictSourceTransport
gdict_source_resolve_transport (const gchar *transport)
{
  if (!transport)
    return GDICT_SOURCE_TRANSPORT_INVALID;
  
  if (strcmp (transport, "dictd") == 0)
    return GDICT_SOURCE_TRANSPORT_DICTD;
  else
    return GDICT_SOURCE_TRANSPORT_INVALID;
  
  g_assert_not_reached ();
}

static void
gdict_source_create_context (GdictSource  *source,
			     const gchar  *transport,
			     GError      **error)
{
  GdictSourcePrivate *priv;
  GdictSourceTransport t;
  GdictContext *context;
  
  g_assert (GDICT_IS_SOURCE (source));
  
  priv = source->priv;
  
  t = gdict_source_resolve_transport (transport);
  switch (t)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      {
      gchar *hostname;
      gint port;
      
      hostname = g_key_file_get_string (priv->keyfile,
      					SOURCE_GROUP,
      					SOURCE_KEY_HOSTNAME,
      					NULL);
      if (!hostname)
        hostname = g_strdup (GDICT_DEFAULT_HOSTNAME);
      
      port = g_key_file_get_integer (priv->keyfile,
      				     SOURCE_GROUP,
      				     SOURCE_KEY_PORT,
      				     NULL);
      if (!port)
        port = GDICT_DEFAULT_PORT;
      
      context = gdict_client_context_new (hostname, port);
      
      g_free (hostname);
      }
      break;
    default:
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("Invalid transport type '%s'"),
                   transport);
      return;
    }

  g_assert (context != NULL);
  
  priv->transport = t;
  priv->context = context;
}

/**
 * gdict_source_load_from_file:
 * @source: an empty #GdictSource
 * @filename: path to a dictionary source file
 * @error: return location for a #GError or %NULL
 *
 * Loads a dictionary source definition file into an empty #GdictSource
 * object.
 *
 * Return value: %TRUE if @filename was loaded successfully.
 *
 * Since: 1.0
 */
gboolean
gdict_source_load_from_file (GdictSource  *source,
			     const gchar  *filename,
			     GError      **error)
{
  GdictSourcePrivate *priv;
  GError *read_error;
  GError *parse_error;
  gchar *transport;
  
  g_return_val_if_fail (GDICT_IS_SOURCE (source), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  
  priv = source->priv;
  
  if (!priv->keyfile)
    priv->keyfile = g_key_file_new ();
  
  read_error = NULL;
  g_key_file_load_from_file (priv->keyfile,
                             filename,
                             G_KEY_FILE_KEEP_TRANSLATIONS,
                             &read_error);
  if (read_error)
    {
      g_propagate_error (error, read_error);
      
      return FALSE;
    }
  
  if (!g_key_file_has_group (priv->keyfile, SOURCE_GROUP))
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("No '%s' group found inside the dictionary source "
                     "definition in '%s'"),
                   SOURCE_GROUP,
                   filename);
      
      return FALSE;
    }
  
  /* fetch the localized name for the dictionary source definition */
  priv->name = g_key_file_get_locale_string (priv->keyfile,
  					     SOURCE_GROUP,
  					     SOURCE_KEY_NAME,
  					     NULL,
  					     &parse_error);
  if (parse_error)
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("Unable to get the '%s' key inside the dictionary "
                     "source definition file '%s': %s"),
                   SOURCE_KEY_NAME,
                   filename,
                   parse_error->message);
      g_error_free (parse_error);
      
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      
      return FALSE;
    }
  
  /* if present, fetch the localized description */
  if (g_key_file_has_key (priv->keyfile, SOURCE_GROUP, SOURCE_KEY_DESCRIPTION, NULL))
    {
      priv->description = g_key_file_get_locale_string (priv->keyfile,
                                                        SOURCE_GROUP,
                                                        SOURCE_KEY_DESCRIPTION,
                                                        NULL,
                                                        &parse_error);
      if (parse_error)
        {
          g_set_error (error, GDICT_SOURCE_ERROR,
                       GDICT_SOURCE_ERROR_PARSE,
                       _("Unable to get the '%s' key inside the dictionary "
                         "source definition file '%s': %s"),
                       SOURCE_KEY_DESCRIPTION,
                       filename,
                       parse_error->message);
                       
          g_error_free (parse_error);
          g_key_file_free (priv->keyfile);
          priv->keyfile = NULL;
          g_free (priv->name);
          
          return FALSE;
        }
    }
  
  transport = g_key_file_get_string (priv->keyfile,
  				     SOURCE_GROUP,
  				     SOURCE_KEY_TRANSPORT,
  				     &parse_error);
  if (parse_error)
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
      		   GDICT_SOURCE_ERROR_PARSE,
      		   _("Unable to get the '%s' key inside the dictionary "
      		     "source definition file '%s': %s"),
      		   SOURCE_KEY_TRANSPORT,
      		   filename,
      		   parse_error->message);
      
      g_error_free (parse_error);
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      g_free (priv->name);
      g_free (priv->description);
      
      return FALSE;
    }
  
  gdict_source_create_context (source, transport, &parse_error);
  if (parse_error)
    {
      g_propagate_error (error, parse_error);
      
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      
      g_free (transport);
      
      g_free (priv->name);
      g_free (priv->description);
      
      return FALSE;
    }
  
  g_assert (priv->context != NULL);
  
  priv->filename = g_strdup (filename);
  
  g_free (transport);
  
  return TRUE;
}

/**
 * gdict_source_to_file:
 * @source: a #GdictSource
 * @filename: FIXME
 * @error: return location for a #GError or %NULL
 *
 * FIXME
 *
 * Return value: %TRUE if the write was successful.
 *
 * Since: 1.0
 */
gboolean
gdict_source_to_file (GdictSource  *source,
		      const gchar  *filename,
		      GError      **error)
{
  g_warning ("%s not yet implemented\n", G_STRFUNC);
  
  return FALSE;
}

/**
 * gdict_source_set_name:
 * @source: a #GdictSource
 * @name: the UTF8-encoded name of the dictionary source
 *
 * Sets @name as the displayable name of the dictionary source.
 *
 * Since: 1.0
 */
void
gdict_source_set_name (GdictSource *source,
		       const gchar *name)
{
  g_warning ("%s not yet implemented\n", G_STRFUNC);
}

/**
 * gdict_source_get_name:
 * @source: a #GdictSource
 *
 * Retrieves the name of @source.
 *
 * Return value: the name of a #GdictSource.  The returned string is owned
 *   by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_name (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->name;
}

/**
 * gdict_source_set_description:
 * @source: a #GdictSource
 * @description: a UTF-8 encoded description or %NULL
 *
 * Sets the description of @source.  If @description is %NULL, unsets the
 * currently set description.
 *
 * Since: 1.0
 */
void
gdict_source_set_description (GdictSource *source,
			      const gchar *description)
{
  g_warning ("%s not yet implemented\n", G_STRFUNC);
}

/**
 * gdict_source_get_description:
 * @source: a #GdictSource
 *
 * Retrieves the description of @source.
 *
 * Return value: the description of a #GdictSource.  The returned string is
 *   owned by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_description (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->description;
}

/**
 * gdict_source_set_transport:
 * @source: a #GdictSource
 * @transport: a valid transport
 *
 * Sets @transport as the choosen transport for @source.  A transport
 * is a method of retrieving dictionary data from a source; it is used
 * to create the right #GdictContext for this #GdictSource.
 *
 * Since: 1.0
 */
void
gdict_source_set_transport (GdictSource          *source,
			    GdictSourceTransport  transport)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  
  g_warning ("%s not yet implemented\n", G_STRFUNC);
}

/**
 * gdict_source_get_transport:
 * @source: a #GdictSource
 *
 * FIXME
 *
 * Return value: FIXME
 *
 * Since: 1.0
 */
GdictSourceTransport
gdict_source_get_transport (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), GDICT_SOURCE_TRANSPORT_INVALID);
  
  return source->priv->transport;
}

/**
 * gdict_source_get_context:
 * @source: a #GdictSource
 *
 * Gets the #GdictContext bound to @source.
 *
 * Return value: a #GdictContext with its reference count increased by one.
 *
 * Since: 1.0
 */
GdictContext *
gdict_source_get_context (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);

  return g_object_ref (source->priv->context);
}

/**
 * gdict_source_set_context:
 * @source: a #GdictSource
 * @context: a #GdictContext
 *
 * FIXME
 *
 * Since: 1.0
 */
void
gdict_source_set_context (GdictSource  *source,
		          GdictContext *context)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  
  g_warning ("%s not yet implemented\n", G_STRFUNC);
}
