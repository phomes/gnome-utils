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

#define _XOPEN_SOURCE 500

#ifdef __CYGWIN__
#define timezonevar
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "logrtns.h"
#include "logview-plugin-manager.h"
#include "logview-debug.h"
#include "misc.h"

#define LOGVIEW_DATA_PLUGIN_PATH LOGVIEWPLUGINDIR
#define LOGVIEW_USER_PLUGIN_PATH_SUFFIX LOGVIEW_HOME_SUFFIX"/plugins"

char *error_main = N_("One file or more could not be opened");
static gboolean queue_err_messages = FALSE;
static GSList *msg_queue_main = NULL, *msg_queue_sec = NULL;
const char *month[12] =
{N_("January"), N_("February"), N_("March"), N_("April"), N_("May"),
 N_("June"), N_("July"), N_("August"), N_("September"), N_("October"),
 N_("November"), N_("December")};

static void
error_dialog_run (GtkWidget *window, const char *main, const char *secondary)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
						     main,
						     secondary);
	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void
error_dialog_show (GtkWidget *window, const char *main, const char *secondary)
{
	if (queue_err_messages) {
		msg_queue_main = g_slist_append (msg_queue_main, g_strdup (main));
		msg_queue_sec = g_slist_append (msg_queue_sec, g_strdup (secondary));
	} else
		error_dialog_run (window, main, secondary);
}

void
error_dialog_queue (gboolean do_queue)
{
	queue_err_messages = do_queue;
}

void
error_dialog_show_queued (void)
{
	if (msg_queue_main != NULL) {
		gboolean title_created = FALSE;
		GSList *li, *li_sec;
		GString *gs = g_string_new (NULL);
		GString *gs_sec = g_string_new (NULL);

		for (li = msg_queue_main, li_sec=msg_queue_sec; li != NULL; li = g_slist_next (li)) {
			char *msg = li->data;
			char *sec = li_sec->data;

			li->data = NULL;
			li_sec->data = NULL;

			if (!title_created) {
				g_string_append (gs, msg);
				title_created = TRUE;
			}
			
			g_string_append (gs_sec, sec);
			if (li->next != NULL)
				g_string_append (gs_sec, "\n");

			g_free (msg);
			g_free (sec);
			li_sec = li_sec->next;
		}
		g_slist_free (msg_queue_main);
		g_slist_free (msg_queue_sec);
		msg_queue_main = NULL;
		msg_queue_sec = NULL;

		error_dialog_run (NULL, gs->str, gs_sec->str);

		g_string_free (gs, TRUE);
		g_string_free (gs_sec, TRUE);
	}
}

char*
error_system_string (void)
{
	return strerror (errno);
}

char *
locale_to_utf8 (const char *in)
{
	char *out;
  
	if (g_utf8_validate (in, -1, NULL))
		out = g_strdup (in);
	else {
		out = g_locale_to_utf8 (in, -1, NULL, NULL, NULL);
		if (out == NULL)
			out = g_strdup ("?");
	}

	return out;
}

GDate *
string_get_date (char *line)
{
	GDate *date;
	struct tm tp;
	char *cp;
	char *tm_locale = NULL;

	if (line == NULL || line[0] == '\0')
		return NULL;
	
	if (tm_locale != NULL) {
change_locale:
		tm_locale = setlocale (LC_TIME, NULL);
		setlocale (LC_TIME, "C");
	}

	if ((cp = strptime (line, "%b %d", &tp)) == NULL)
		if ((cp = strptime (line, "%a %b %d", &tp)) == NULL)
			if ((cp = strptime (line, "%F", &tp)) == NULL) {
				if (tm_locale == NULL)
					goto change_locale;
				else {
					setlocale (LC_TIME, tm_locale);
					return NULL;
				}
			}

	if (tm_locale != NULL) {
		setlocale (LC_TIME, tm_locale);
	}

	date = g_date_new_dmy (tp.tm_mday, tp.tm_mon+1, 70);
	return date;
}

char *
date_to_string (GDate *date)
{
	char buf[512];
	char *utf8;

	if (date == NULL || !g_date_valid (date)) {
		utf8 = g_strdup(_("Invalid date"));
		return utf8;
	}
   
	/* Translators: Only date format, time will be bogus */
	if (g_date_strftime (buf, sizeof (buf), _("%x"), date) == 0) {
		int m = g_date_get_month (date);
		int d = g_date_get_day (date);
		/* If we fail just use the US format */
		utf8 = g_strdup_printf ("%s %d", _(month[(int) m-1]), d);
	} else
		utf8 = locale_to_utf8 (buf);
   
	return utf8;
}

/* File checking */

gboolean
try_to_open (const gchar *filename, gboolean show_error)
{
	GnomeVFSHandle *handle;
	GnomeVFSResult result;
	char *secondary = NULL;

	if (filename == NULL)
	   return FALSE;

	result = gnome_vfs_open (&handle, filename, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
	   if (show_error) {
		   switch (result) {
		   case GNOME_VFS_ERROR_ACCESS_DENIED:
		   case GNOME_VFS_ERROR_NOT_PERMITTED:
			   secondary = g_strdup_printf (_("%s is not user readable. "
					 "Either run the program as root or ask the sysadmin to "
					 "change the permissions on the file.\n"), filename);
			   break;
		   case GNOME_VFS_ERROR_TOO_BIG:
			   secondary = g_strdup_printf (_("%s is too big."), filename);
			   break;
		   default:
			   secondary = g_strdup_printf (_("%s could not be opened."), filename);
			   break;
		   }
		   error_dialog_show (NULL, error_main, secondary);
		   g_free (secondary);
	   }
	   return FALSE;
	}

	gnome_vfs_close (handle);
	return TRUE;
}

gchar *
log_extract_dirname (const gchar *logname)
{
	GnomeVFSURI *uri;
	gchar *dirname;

	uri = gnome_vfs_uri_new (logname);
	dirname = gnome_vfs_uri_extract_dirname (uri);
	gnome_vfs_uri_unref (uri);

	return dirname;
}

gchar *
log_extract_filename (const gchar *logname)
{
	GnomeVFSURI *uri;
	gchar *filename;

	uri = gnome_vfs_uri_new (logname);
	filename = gnome_vfs_uri_extract_short_name (uri);
	gnome_vfs_uri_unref (uri);

	return filename;
}

void
extract_filepath (const gchar *logname, gchar** dirname, gchar** filename)
{
	GnomeVFSURI *uri;

	uri = gnome_vfs_uri_new (logname);
	if (dirname != NULL) *dirname = gnome_vfs_uri_extract_dirname (uri);
	if (filename != NULL) *filename = gnome_vfs_uri_extract_short_name (uri);
	gnome_vfs_uri_unref (uri);	
}

gchar *
get_date_string (gchar *line)
{
	gchar **split, *date_string;
	gchar *month=NULL, *day=NULL;
	int i=0;

	if (line == NULL || line[0] == 0)
		return NULL;

	split = g_strsplit (line, " ", 4);
	if (split == NULL)
		return NULL;

	while ((day == NULL || month == NULL) && split[i]!=NULL && i<4) {
		if (g_str_equal (split[i], "")) {
			i++;
			continue;
		}

		if (month == NULL) {
			month = split[i++];
			/* If the first field begins by a number, the date
			   is given in yyyy-mm-dd format */
			if (!g_ascii_isalpha (month[0]))
				break;
			continue;
		}

		if (day == NULL)
			day = split[i];
		i++;
	}

	if (i==3)
		date_string = g_strconcat (month, "  ", day, NULL);
	else
		date_string = g_strconcat (month, " ", day, NULL);
	g_strfreev (split);
	return (date_string);
}

LogProtocol
file_protocol (const char *filename)
{
	LogProtocol protocol;

	LV_MARK;
	g_assert (filename);
	g_assert (filename[0] != '\0');
	
	if (local_file_exist (filename)) {
		char *mime_type = gnome_vfs_get_mime_type (filename);
		LV_INFO ("[protocol] %s detected mime: %s",
			 filename,
			 mime_type != NULL ? mime_type : "(nil)");
		if (mime_type == NULL) {
			protocol = UNKNOWN_LOG;
		} else if (g_str_has_prefix (mime_type, "text/")) {
			protocol = PLAIN_LOG;
		} else if (strcmp (mime_type, "application/octet-stream")==0) {
			protocol =  PIPE_LOG;
		} else if (strcmp (mime_type, "application/x-gzip")==0 ||
			   strcmp (mime_type, "application/x-zip")==0 ||
			   strcmp (mime_type, "application/zip")==0) {
			protocol =  GZIP_LOG;
		} else if (g_str_has_prefix (mime_type, "x-directory/") ||
			   g_str_has_prefix (mime_type, "application/x-executable")) {
			protocol = BAD_LOG;
		} else {
			protocol = UNKNOWN_LOG;
		}
		g_free (mime_type);
	} else {
		protocol = UNKNOWN_LOG;
	}
	LV_INFO ("[protocol] %s assigned protocol: %d", filename, protocol);
	return protocol;
}

gboolean
local_file_can_executed (const gchar *filename)
{
	struct stat buf;
	if (stat (filename, &buf) == 0) {
		return buf.st_uid == getuid() || buf.st_uid == 0;
	}
}

gboolean
local_file_exist (const gchar *filename)
{
	struct stat buf;
	if (stat (filename, &buf) == 0) {
		return S_ISBLK (buf.st_mode) ||
			S_ISCHR (buf.st_mode) ||
			S_ISDIR (buf.st_mode) ||
			S_ISFIFO (buf.st_mode) ||
			S_ISREG (buf.st_mode) ||
			S_ISSOCK (buf.st_mode);
	}
	return FALSE;
}

gchar *
processor_arch (void)
{
	static gchar cpu_arch[BUFSIZ] = {0};
	/* get system ARCH for Solaris*/
	if (*cpu_arch == 0) {
#ifdef ON_SUN_OS
#include <sys/systeminfo.h>
	if (sysinfo(SI_ARCHITECTURE, cpu_arch, sizeof (cpu_arch)) == -1) {
		LV_INFO_EE ("get sysinfo failed: %s", error_system_string ());
	}
#else
#include <sys/utsname.h>
	struct utsname buf;
	if (uname(&buf) == -1) {
		LV_INFO_EE ("get sysinfo failed: %s", error_system_string ());
	}
	strcpy(cpu_arch, buf.machine);
#endif
	}
	LV_ERR ("ARCH = %s", cpu_arch);
	return cpu_arch;
}

G_CONST_RETURN GSList*
get_plugin_paths()
{
	static GSList *plugin_paths = NULL;
	struct stat buf;
	gchar* user_plugin_path = NULL;

	if (plugin_paths)
		return plugin_paths;

	/* get user plugin path */
	user_plugin_path = g_build_path (G_DIR_SEPARATOR_S,
					 (gchar*)g_get_home_dir (),
					 LOGVIEW_USER_PLUGIN_PATH_SUFFIX,
					 processor_arch (),
					 NULL);
	if (stat (user_plugin_path, &buf) == 0) {
		if (S_ISDIR (buf.st_mode)) {
			plugin_paths = g_slist_append (plugin_paths,
						       user_plugin_path);
		} else {
			g_free (user_plugin_path);
		}
	}
	else if (errno == ENOENT &&
		 g_mkdir_with_parents (user_plugin_path, get_umask()) == 0) {
			plugin_paths = g_slist_append (plugin_paths,
						       user_plugin_path);
	}
	else {
		LV_ERR ("err|create user plugin path: %s", user_plugin_path);
		g_free (user_plugin_path);
		return NULL;
	}
	
	/* get default plugin path */
	if (stat (LOGVIEW_DATA_PLUGIN_PATH, &buf) == 0) {
		if (S_ISDIR (buf.st_mode)) {
			plugin_paths = g_slist_append(plugin_paths,
						      (gpointer*)g_strdup (LOGVIEW_DATA_PLUGIN_PATH));
		}
	}
	return plugin_paths;
}

/*
 * Alternative popen due to the security issue
 */

typedef struct {
	int fd;
	pid_t pid;
} pf_node; /* pair of pipe and file description */

static GSList *pf_list = NULL;
static void
pf_insert (int fd, pid_t pid)
{
	pf_node *node = g_new0 (pf_node, 1);
	g_assert (node);
	node->fd = fd;
	node->pid = pid;
	pf_list = g_slist_prepend (pf_list, node);
}

static pid_t
pf_delete (int fd)
{
	GSList *idx = NULL;
	pid_t pid = -1;
	for (idx = pf_list; idx; idx=g_slist_next (idx)) {
		if (((pf_node *)idx->data)->fd == fd) {
			pid = ((pf_node *)idx->data)->pid;
			g_free (idx->data);
			pf_list = g_slist_delete_link (pf_list, idx);
			break;
		}
	}
	return pid;
}

static GStaticMutex pipe_mutex = G_STATIC_MUTEX_INIT;
#define PIPE_MUTEX_LOCK() (g_static_mutex_lock (&pipe_mutex))
#define PIPE_MUTEX_UNLOCK() (g_static_mutex_unlock (&pipe_mutex))
/**
 * _security_popen_raw:
 * @cmd: 
 * @argv: directly pass to execve, so should be NULL terminated.
 * @envp: directly pass to execve.
 * 
 * Only support read.
 * 
 * Note that, the path is restricted in "/usr/bin:/usr/sbin".
 * 
 * RETURNS: the pointer of the file stream structure.
 */
static FILE*
_security_popen (const char *path, char *const argv[], char *const envp[])
{
	int p[2];
	pid_t pid;
	FILE* iop;

	if (path == NULL || argv == NULL)
		return NULL;

	if (pipe (p) < 0)
		return NULL;

	if ((iop = fdopen (p[0], "r")) == NULL) {
		close (p[0]);
		close (p[1]);
		return NULL;
	}

	PIPE_MUTEX_LOCK();

	if ((pid = vfork()) == 0) {
		/* child */
		GSList *idx = NULL;
		int error;
		close (p[0]);
		if (p[1] != STDOUT_FILENO) {
			if (dup2 (p[1], STDOUT_FILENO) == -1){
				perror ("dup2(3C)");
				exit (errno);
			}
			close (p[1]);
		}

		/* close all fds */
		for (idx = pf_list; idx; idx = g_slist_next (idx)) {
			close (((pf_node *)idx->data)->fd);
			g_free (idx->data);
		}
		g_slist_free (pf_list);

		error = execve (path, argv, envp);
		if ( error == -1) {
			perror ("execve(2)");
			exit (errno);
		}
		exit (127);
	}
	else if (pid < 0) {
		PIPE_MUTEX_UNLOCK();
		close (p[0]);
		close (p[1]);
		fclose (iop);
		return NULL;
	}
	/* parent */
	PIPE_MUTEX_UNLOCK();
	close (p[1]);
	pf_insert (p[0], pid);
	return iop;
}

/**
 * security_pclose:
 * @fp:
 * 
 */
int
security_pclose (FILE *fp)
{
#ifdef ON_SUN_OS
	pid_t pid;
	int status;

	if (fp == NULL)
		return -1;

	pid = pf_delete (fileno (fp));

	fclose (fp);
	if (pid == -1)
		return -1;
        while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) {
                        status = -1;
                        break;
                }
        }
        return status;
#else
	return pclose (fp);
#endif
}

/**
 * security_popen:
 * @command: the command need be executed.
 * @mode: for compatible usage in Linux, this function will call popen().
 * In Solaris, it's useless.
 * 
 * Only support read.
 * 
 * Note that, the path is restricted in "/bin:/sbin:/usr/bin:/usr/sbin".
 * 
 * RETURNS: the pointer of the file stream structure.
 */
FILE*
security_popen (const char *command, const char *mode)
{
#ifdef ON_SUN_OS
	static char *const res_path[] = {"PATH=/bin:/sbin:/usr/bin:/usr/sbin", NULL};
	static const char *shellenv = "SHELL";
	static const char *pfsh_path = "/bin/pfsh";
	static const char *pfsh = "pfsh";
	static const char *pfcsh_path = "/bin/pfcsh";
	static const char *pfcsh = "pfcsh";
	static const char *pfksh_path = "/bin/pfksh";
	static const char *pfksh = "pfksh";
	static const char *sh_opt = "-c";
	char *argve[4];
	char *shpath = NULL;
	char *shell = NULL;

	shpath = (char*) pfsh_path;
	shell = (char*) pfsh;
	argve[0] = (char *) shell;
	argve[1] = (char *) sh_opt;
	argve[2] = (char *) command;
	argve[3] = NULL;
	return _security_popen (shpath, (char* const*)argve, res_path);
#else
	return popen (command, mode);
#endif
}

static gchar *err_msg = NULL;
G_CONST_RETURN gchar *
log_error (void)
{
	return err_msg;
}

void
log_error_init (const gchar *log_path)
{
	if (err_msg)
		g_free (err_msg);
	err_msg = g_strdup_printf ("%s :\n", log_path);
}

void
log_error_append (const gchar *msg)
{
	gchar *tmp;
	g_assert (msg);
	tmp = g_strdup_printf ("%s\t%s\n", err_msg, msg);
	g_free (err_msg);
	err_msg = tmp;
}

void
log_error_appendv (const gchar *format, ...)
{
	gchar *tmp, *err;
	va_list ap;

	g_assert (format);

	va_start (ap, format);
	err= g_strdup_vprintf (format, ap);
	va_end (ap);

	tmp = g_strdup_printf ("%s\t%s\n", err_msg, err);

	g_free (err);
	g_free (err_msg);
	err_msg = tmp;
}

gint
get_umask ()
{
	static gint current_umask=0;
	if (current_umask == 0) {
		mode_t umode = 0;
		umode = umask (umode);
		umask (umode);
		current_umask = umode ^ 0777;
	}
	return current_umask;
}

gboolean
create_home_dir (void)
{
	struct stat buf;
	gboolean ret = FALSE;
	gchar* logview_home = NULL;

	/* get user plugin path */
	logview_home = g_build_path (G_DIR_SEPARATOR_S,
					 (gchar*)g_get_home_dir (),
					 LOGVIEW_HOME_SUFFIX,
					 NULL);
	if (stat (logview_home, &buf) == 0) {
		if (S_ISDIR (buf.st_mode) && (S_IRWXU & buf.st_mode)) {
			ret = TRUE;
		}
	} else if (errno == ENOENT &&
		   g_mkdir_with_parents (logview_home, get_umask()) == 0) {
		ret = TRUE;
	}
	return ret;
}

