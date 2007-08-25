/*  ----------------------------------------------------------------------

    Copyright (C) 1998  Cesar Miquel  (miquel@df.uba.ar)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#ifndef __LOGRTNS_H__
#define __LOGRTNS_H__

#include <libgnomevfs/gnome-vfs.h>
#include "logview-log.h"

G_BEGIN_DECLS

/* call by log_repaint */
Day * log_find_day (Log *self, const GDate* date);

/* log utilities base on the plugin interfaces */
void log_extract_filepath (Log* self, gchar** dirname, gchar** filename);
gboolean log_has_been_modified (Log* self);
gboolean log_run (Log* self);

G_END_DECLS

#endif /* __LOGRTNS_H__ */
