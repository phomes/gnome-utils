/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LOG_MISC_H__
#define __LOG_MISC_H__

#include "logview-log.h"

#define LOGVIEW_HOME_SUFFIX ".gnome2/gnome-system-log"

void error_dialog_show (GtkWidget *window, char *main, char *secondary);
void error_dialog_queue (gboolean do_queue);
void error_dialog_show_queued (void);
char* error_system_string (void);

char *locale_to_utf8 (const char *in);
GDate *string_get_date (char *line);
int string_get_month (const char *str);
char *date_to_string (GDate *date);

gboolean file_is_log (char *filename, gboolean show_error);
gchar *log_extract_dirname (const gchar *logname);
gchar *log_extract_filename (const gchar *logname);

void extract_filepath (const gchar *logname, gchar** dirname, gchar** filename);
LogProtocol file_protocol (const char *filename);
gboolean try_to_open (const gchar *filename, gboolean show_error);
gboolean local_file_exist (const gchar *filename);
gboolean local_file_can_executed (const gchar *filename);

gchar * get_date_string (gchar *line);
gchar * processor_arch (void);
G_CONST_RETURN GSList* get_plugin_paths();
FILE* security_popen (const char *cmd, const char *mode);
int security_pclose (FILE *fp);

G_CONST_RETURN gchar *log_error (void);
void log_error_append (const gchar *msg);
void log_error_init (const gchar *log_path);
gint get_umask ();
gboolean create_home_dir (void);

#endif /* __LOG_MISC_H__ */

