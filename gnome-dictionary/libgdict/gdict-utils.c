/* gdict-utils.c - Utility functions for Gdict
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#include "gdict-utils.h"

#ifdef GDICT_ENABLE_DEBUG
void
gdict_debug (const gchar *fmt,
             ...)
{
  va_list args;

  fprintf (stderr, "GDICT_DEBUG: ");
    
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
}
#endif

/* gdict_has_ipv6: checks for the existence of the IPv6 extensions; if
 * IPv6 support was not enabled, this function always return false
 */
gboolean
gdict_has_ipv6 (void)
{
#ifdef ENABLE_IPV6
  int s;

  s = socket (AF_INET6, SOCK_STREAM, 0);
  if (s != -1)
    {
      close(s);
      
      return TRUE;
    }
#endif

  return FALSE;
}
