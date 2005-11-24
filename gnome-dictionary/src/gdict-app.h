/* gdict-app.h - main application singleton
 *
 * This file is part of GNOME Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __GDICT_APP_H__
#define __GDICT_APP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GDICT_TYPE_APP		(gdict_app_get_type ())
#define GDICT_APP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_APP, GdictApp))
#define GDICT_IS_APP(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_APP))

typedef struct _GdictApp        GdictApp;
typedef struct _GdictAppClass   GdictAppClass;
typedef struct _GdictAppPrivate GdictAppPrivate;

struct _GdictApp
{
  GObject parent_instance;
  
  GdictAppPrivate *priv;
};


GType     gdict_app_get_type (void) G_GNUC_CONST;
GdictApp *gdict_app_new      (void);

void      gdict_app_run      (GdictApp *app);

G_END_DECLS

#endif /* __GDICT_APP_H__ */
