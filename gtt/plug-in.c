/*   Report Plugins for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <glib.h>

#include "plug-in.h"


/* ============================================================ */

static GList *plugin_list = NULL;

GList * 
gtt_plugin_get_list (void)
{
	return plugin_list;
}

GttPlugin *
gtt_plugin_new (const char * nam, const char * pth)
{
	GttPlugin *plg;

	if (!nam || !pth) return NULL;

	plg = g_new (GttPlugin, 1);
	plg->name = g_strdup(nam);
	plg->path = g_strdup(pth);

	plugin_list = g_list_append (plugin_list, plg);

	return plg;
}

/* ============================================================ */
