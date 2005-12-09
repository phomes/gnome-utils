/* gdict-window.c - main application window
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/goption.h>
#include <glib/gi18n.h>
#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include "gdict.h"

#include "gdict-pref-dialog.h"
#include "gdict-about.h"
#include "gdict-window.h"

#define GDICT_GCONF_DIR	  		"/apps/gnome-dictionary"
#define GDICT_GCONF_PRINT_FONT_KEY 	GDICT_GCONF_DIR "/print_font"

#define GDICT_WINDOW_MIN_WIDTH	  400
#define GDICT_WINDOW_MIN_HEIGHT	  330

#define GDICT_DEFAULT_PRINT_FONT  "Serif 10"

enum
{
  PROP_0,
  
  PROP_SOURCE_LOADER,
  PROP_WINDOW_ID,
  PROP_PRINT_FONT,
  PROP_WORD
};

enum
{
  CREATED,
  
  LAST_SIGNAL
};

static guint gdict_window_signals[LAST_SIGNAL] = { 0 };



G_DEFINE_TYPE (GdictWindow, gdict_window, GTK_TYPE_WINDOW);



static void
gdict_window_finalize (GObject *object)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  if (window->context)
    {
      g_signal_handler_disconnect (window->context, window->lookup_start_id);
      g_signal_handler_disconnect (window->context, window->lookup_end_id);
      g_signal_handler_disconnect (window->context, window->error_id);

      g_object_unref (window->context);
    }
  
  if (window->loader)
    g_object_unref (window->loader);
  
  if (window->ui_manager)
    g_object_unref (window->ui_manager);
  
  g_object_unref (window->action_group);
  
  g_free (window->print_font);
  g_free (window->word);
  
  G_OBJECT_CLASS (gdict_window_parent_class)->finalize (object);
}

static void
gdict_window_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      if (window->loader)
        g_object_unref (window->loader);
      window->loader = g_value_get_object (value);
      g_object_ref (window->loader);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_window_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, window->loader);
      break;
    case PROP_WINDOW_ID:
      g_value_set_uint (value, window->window_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_window_cmd_file_new (GtkAction   *action,
			   GdictWindow *window)
{
  GtkWidget *new_window;
  
  new_window = gdict_window_new (window->loader);
  gtk_widget_show (new_window);
  
  g_signal_emit (window, gdict_window_signals[CREATED], 0, new_window);
}

static void
gdict_window_cmd_save_as (GtkAction   *action,
			  GdictWindow *window)
{
  GtkWidget *dialog;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  dialog = gtk_file_chooser_dialog_new (_("Save a Copy"),
  					GTK_WINDOW (window),
  					GTK_FILE_CHOOSER_ACTION_SAVE,
  					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
  					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  
  /* default to user's home */
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("Untitled document"));
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;
      gchar *text;
      gsize len;
      GError *write_error = NULL;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      
      text = gdict_defbox_get_text (GDICT_DEFBOX (window->defbox), &len);
      
      g_file_set_contents (filename,
      			   text,
      			   len,
      			   &write_error);
      if (write_error)
        {
          g_warning ("Error while writing to '%s': %s\n",
                     filename,
                     write_error->message);
          
          g_error_free (write_error);
        }
      
      g_free (text);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
get_page_margins (GnomePrintConfig *config,
		  gdouble          *top_margin,
		  gdouble          *right_margin,
		  gdouble          *bottom_margin,
		  gdouble          *left_margin)
{
  g_assert (config != NULL);

  gnome_print_config_get_length (config,
  				 GNOME_PRINT_KEY_PAGE_MARGIN_TOP, top_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT, right_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM, bottom_margin,
  				 NULL);
  gnome_print_config_get_length (config,
  				 GNOME_PRINT_KEY_PAGE_MARGIN_LEFT, left_margin,
  				 NULL);
}

static gboolean
gdict_window_prepare_print (GdictWindow   *window,
			    GnomePrintJob *job)
{
  GnomePrintContext *pc;
  GnomePrintConfig *config;
  PangoLayout *layout;
  PangoFontDescription *font_desc;
  gdouble page_width, page_height;
  gdouble top_margin, right_margin, bottom_margin, left_margin;
  gint lines_per_page, total_lines, written_lines, remaining_lines;
  gint line_width;
  gint num_pages;
  gint x, y, i;
  gchar *text, *header_line;
  gchar **text_lines;
  gsize length;
  
  g_assert (GDICT_IS_WINDOW (window));
  g_assert (job != NULL);
  
  pc = gnome_print_job_get_context (job);
  if (!pc)
    {
      g_warning ("Unable to create a GnomePrintContext for the job\n");
      
      return FALSE;
    }
  
  /* take the defbox text, and break it up into lines */
  total_lines = 0;
  text = gdict_defbox_get_text (GDICT_DEFBOX (window->defbox), &length);
  if (!text)
    {
      g_object_unref (pc);
      
      return TRUE;
    }
  
  text_lines = g_strsplit (text, "\n", -1);
  header_line = text_lines[0];
  remaining_lines = total_lines = g_strv_length (text_lines);
  
  layout = gnome_print_pango_create_layout (pc);
  
  if (!window->print_font)
    window->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  font_desc = pango_font_description_from_string (window->print_font);
  line_width = pango_font_description_get_size (font_desc) / PANGO_SCALE;
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);
  
  config = gnome_print_job_get_config (job);
  gnome_print_job_get_page_size_from_config (config,
                                             &page_width,
                                             &page_height);
  get_page_margins (config, &top_margin, &right_margin, &bottom_margin, &left_margin);
  
  pango_layout_set_width (layout, (page_width - left_margin - right_margin) * PANGO_SCALE);
  
  lines_per_page = MIN (((page_height - top_margin - bottom_margin) / line_width), total_lines);
  num_pages = ceil ((float) total_lines / (float) lines_per_page);
  
  /* the first line of the buffer is the queried word; we print it on
   * every page, so we must skip it from the lines buffer and skip it
   * from the lines per page count
   */
  written_lines = 1;
  lines_per_page -= 4;
  
  x = left_margin;
  for (i = 0; i < num_pages; i++)
    {
      gchar *page;
      gchar *header;
      gint j;
      
      if (written_lines == total_lines)
        break;
      
      page = g_strdup_printf ("%d", i + 1);
      gnome_print_beginpage (pc, page);
      
      y = page_height - top_margin;
      
      /* print header + line */
      gnome_print_moveto (pc, x, y);
      
      header = g_strdup_printf (_("%s (page %d)"), header_line, i + 1);
      pango_layout_set_text (layout, header, -1);
      gnome_print_pango_layout (pc, layout);
      
      y -= 2 * line_width;
      
      gnome_print_moveto (pc, x, y);
      gnome_print_lineto (pc, x + (page_width - right_margin - left_margin), y);
      gnome_print_stroke (pc);
      
      y -= 2 * line_width;
      
      for (j = 0; j < lines_per_page; j++)
        {
          gchar *line = text_lines[written_lines++];
          
          if (!line)
            break;
          
          gnome_print_moveto (pc, x, y);
          
          pango_layout_set_text (layout, line, -1);
          gnome_print_pango_layout (pc, layout);
          
          remaining_lines -= 1;
          y -= line_width;
        }
      
      g_free (page);
      g_free (header);
      
      gnome_print_showpage (pc);
    }
  
  g_object_unref (pc);

  g_strfreev (text_lines);
  g_free (text);
  
  gnome_print_job_close (job);
  
  return TRUE;
}

static void
gdict_window_show_print_preview (GdictWindow   *window,
				 GnomePrintJob *job)
{
  GtkWidget *preview;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  preview = gnome_print_job_preview_new (job, _("Print Preview"));
  gtk_window_set_transient_for (GTK_WINDOW (preview), GTK_WINDOW (window));
  
  gtk_widget_show (preview);
}

static void
gdict_window_print_response_cb (GtkWidget *widget,
				gint       response_id,
				gpointer   user_data)
{
  GdictWindow *window = GDICT_WINDOW (user_data);
  GnomePrintJob *job;
  
  job = g_object_get_data (G_OBJECT (widget), "print-job");
  
  switch (response_id)
    {
    case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
      if (gdict_window_prepare_print (window, job))
        {
          gint collate, copies, i;
          
          gnome_print_dialog_get_copies (GNOME_PRINT_DIALOG (widget),
          				 &copies,
          				 &collate);
          
          for (i = 0; i < copies; i++)
            gnome_print_job_print (job);
        }
      break;
    case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
      if (gdict_window_prepare_print (window, job));
        gdict_window_show_print_preview (window, job);
      break;
    case GNOME_PRINT_DIALOG_RESPONSE_CANCEL:
    default:
      break;
    }
}

static void
gdict_window_cmd_file_print (GtkAction   *action,
			     GdictWindow *window)
{
  GtkWidget *print_dialog;
  GnomePrintJob *job;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  job = gnome_print_job_new (NULL);
  
  print_dialog = gnome_print_dialog_new (job, _("Print"), 0);
  g_object_set_data (G_OBJECT (print_dialog), "print-job", job);
  
  g_signal_connect (print_dialog, "response",
      		    G_CALLBACK (gdict_window_print_response_cb),
      		    window);
  
  gtk_dialog_run (GTK_DIALOG (print_dialog));
  
  gtk_widget_destroy (print_dialog);
  
  g_object_unref (job);
}

static void
gdict_window_cmd_file_close_window (GtkAction   *action,
				    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gtk_widget_destroy (GTK_WIDGET (window));
}

static void
gdict_window_cmd_edit_copy (GtkAction   *action,
			    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_copy_to_clipboard (GDICT_DEFBOX (window->defbox),
		  		  gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

static void
gdict_window_cmd_edit_select_all (GtkAction   *action,
				  GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_select_all (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_find (GtkAction   *action,
			    GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_defbox_set_show_find (GDICT_DEFBOX (window->defbox), TRUE);
}

static void
gdict_window_cmd_edit_find_next (GtkAction   *action,
				 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_find_next (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_find_previous (GtkAction   *action,
				     GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gdict_defbox_find_previous (GDICT_DEFBOX (window->defbox));
}

static void
gdict_window_cmd_edit_preferences (GtkAction   *action,
				   GdictWindow *window)
{

}

static void
gdict_window_cmd_go_first_def (GtkAction   *action,
			       GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  window->last_definition = 0;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_previous_def (GtkAction   *action,
				  GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->last_definition == 0)
    return;
  
  window->last_definition -= 1;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_next_def (GtkAction   *action,
			      GdictWindow *window)
{
  gint max;
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->max_definition == -1)
    window->max_definition = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox)) - 1;
    
  if (window->last_definition == window->max_definition)
    return;
  
  window->last_definition += 1;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_go_last_def (GtkAction   *action,
			      GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  if (window->max_definition == -1)
    window->last_definition = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox)) - 1;
  
  window->last_definition = window->max_definition;
  gdict_defbox_jump_to_definition (GDICT_DEFBOX (window->defbox),
                                   window->last_definition);
}

static void
gdict_window_cmd_help_contents (GtkAction   *action,
				GdictWindow *window)
{

}

static void
gdict_window_cmd_help_about (GtkAction   *action,
			     GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_show_about_dialog (GTK_WIDGET (window));
}

static void
gdict_window_cmd_lookup (GtkAction   *action,
			 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));

  gtk_widget_grab_focus (window->entry);
}

static void
gdict_window_cmd_escape (GtkAction   *action,
			 GdictWindow *window)
{
  g_assert (GDICT_IS_WINDOW (window));
  
  gdict_defbox_set_show_find (GDICT_DEFBOX (window->defbox), FALSE);
}

static const GtkActionEntry entries[] =
{
  { "File", NULL, N_("_File") },
  { "Edit", NULL, N_("_Edit") },
  { "Go", NULL, N_("_Go") },
  { "Help", NULL, N_("_Help") },

  /* File menu */
  { "FileNew", GTK_STOCK_NEW, N_("_New"), "<control>N",
    N_("New look up"), G_CALLBACK (gdict_window_cmd_file_new) },
  { "FileSaveAs", GTK_STOCK_SAVE_AS, N_("_Save a Copy..."), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_save_as) },
  { "FilePrint", GTK_STOCK_PRINT, N_("_Print..."), "<control>P",
    N_("Print this document"), G_CALLBACK (gdict_window_cmd_file_print) },
  { "FileCloseWindow", GTK_STOCK_CLOSE, NULL, "<control>W", NULL,
    G_CALLBACK (gdict_window_cmd_file_close_window) },

  /* Edit menu */
  { "EditCopy", GTK_STOCK_COPY, NULL, "<control>C", NULL,
    G_CALLBACK (gdict_window_cmd_edit_copy) },
  { "EditSelectAll", NULL, N_("Select _All"), "<control>A", NULL,
    G_CALLBACK (gdict_window_cmd_edit_select_all) },
  { "EditFind", GTK_STOCK_FIND, NULL, "<control>F",
    N_("Find a word or phrase in the document"),
    G_CALLBACK (gdict_window_cmd_edit_find) },
  { "EditFindNext", NULL, N_("Find Ne_xt"), "<control>G", NULL,
    G_CALLBACK (gdict_window_cmd_edit_find_next) },
  { "EditFindPrevious", NULL, N_("Find Pre_vious"), "<control><shift>G", NULL,
    G_CALLBACK (gdict_window_cmd_edit_find_previous) },
  { "EditPreferences", NULL, N_("_Preferences"), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_edit_preferences) },

  /* Go menu */
  { "GoPreviousDef", GTK_STOCK_GO_BACK, N_("_Previous Definition"), "<control>Page_Up",
    N_("Go to the previous definition"), G_CALLBACK (gdict_window_cmd_go_previous_def) },
  { "GoNextDef", GTK_STOCK_GO_FORWARD, N_("_Next Definition"), "<control>Page_Down",
    N_("Go to the next definition"), G_CALLBACK (gdict_window_cmd_go_next_def) },
  { "GoFirstDef", GTK_STOCK_GOTO_FIRST, N_("_First Definition"), "<control>Home",
    N_("Go to the first definition"), G_CALLBACK (gdict_window_cmd_go_first_def) },
  { "GoLastDef", GTK_STOCK_GOTO_LAST, N_("_Last Definition"), "<control>End",
    N_("Go to the last definition"), G_CALLBACK (gdict_window_cmd_go_last_def) },

  /* Help menu */
  { "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", NULL,
    G_CALLBACK (gdict_window_cmd_help_contents) },
  { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL,
    G_CALLBACK (gdict_window_cmd_help_about) },
  
  /* Accellerators */
  { "Lookup", NULL, "", "<control>L", NULL, G_CALLBACK (gdict_window_cmd_lookup) },
  { "Escape", NULL, "", "Escape", "", G_CALLBACK (gdict_window_cmd_escape) },
  { "Slash", GTK_STOCK_FIND, NULL, "slash", NULL, G_CALLBACK (gdict_window_cmd_edit_find) },
};

static void
gdict_window_lookup_start_cb (GdictContext *context,
			      GdictWindow  *window)
{
  gchar *message;

  if (!window->word)
    return;

  message = g_strdup_printf (_("Searching for '%s'..."), window->word);
  
  if (window->status)
    gtk_statusbar_push (GTK_STATUSBAR (window->status), 0, message);

  g_free (message);
}

static void
gdict_window_lookup_end_cb (GdictContext *context,
			    GdictWindow  *window)
{
  gchar *message;
  gint count;

  count = gdict_defbox_count_definitions (GDICT_DEFBOX (window->defbox));

  if (window->max_definition == -1)
    window->max_definition = count;

  message = g_strdup_printf (_("%d definitions found"), count);

  if (window->status)
    gtk_statusbar_push (GTK_STATUSBAR (window->status), 0, message);

  g_free (message);
}

static void
gdict_window_error_cb (GdictContext *context,
		       GError       *error,
		       GdictWindow  *window)
{
  if (window->word)
    g_free (window->word);
}

static GdictContext *
get_context_from_loader (GdictWindow *window)
{
  GdictSource *source;
  GdictContext *retval;
  
  source = gdict_source_loader_get_source (window->loader, _("Default"));
  if (!source)
    {
      g_warning ("Unable to find a dictionary source");
      return NULL;
    }
  
  retval = gdict_source_get_context (source);

  /* attach our callbacks */
  window->lookup_start_id = g_signal_connect (retval, "lookup-start",
		  			      G_CALLBACK (gdict_window_lookup_start_cb),
					      window);
  window->lookup_end_id = g_signal_connect (retval, "lookup-end",
		  			    G_CALLBACK (gdict_window_lookup_end_cb),
					    window);
  window->error_id = g_signal_connect (retval, "error",
		  		       G_CALLBACK (gdict_window_error_cb),
				       window);
  
  g_object_unref (source);
  
  return retval;
}

static void
entry_activate_cb (GtkWidget   *widget,
		   GdictWindow *window)
{
  const gchar *word;
  gchar *title;
  
  g_assert (GDICT_IS_WINDOW (window));
  
  word = gtk_entry_get_text (GTK_ENTRY (widget));
  if (!word)
    return;
  
  g_free (window->word);
  window->word = g_strdup (word);
  
  title = g_strdup_printf (_("Dictionary - %s"), window->word);
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_free (title);
  
  window->max_definition = -1;
  window->last_definition = 0;
  
  gdict_defbox_lookup (GDICT_DEFBOX (window->defbox), window->word);
}

static GObject *
gdict_window_constructor (GType                  type,
			  guint                  n_construct_properties,
			  GObjectConstructParam *construct_params)
{
  GObject *object;
  GdictWindow *window;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error;
  
  object = G_OBJECT_CLASS (gdict_window_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  
  window = GDICT_WINDOW (object);
  
  gtk_widget_push_composite_child ();
  
  gtk_window_set_title (GTK_WINDOW (window), _("Dictionary"));
  gtk_window_set_default_size (GTK_WINDOW (window),
  			       GDICT_WINDOW_MIN_WIDTH,
  			       GDICT_WINDOW_MIN_HEIGHT);
  
  window->main_box = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (window), window->main_box);
  gtk_widget_show (window->main_box);
  
  /* build menus */
  action_group = gtk_action_group_new ("MenuActions");
  window->action_group = action_group;
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, entries,
  				G_N_ELEMENTS (entries),
  				window);
  
  window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->ui_manager, action_group, 0);
  
  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_file (window->ui_manager,
  					PKGDATADIR "/gnome-dictionary-ui.xml",
  					&error))
    {
      g_warning ("Building menus failed: %s", error->message);
      g_error_free (error);
    }
  else
    {
      window->menubar = gtk_ui_manager_get_widget (window->ui_manager, "/MainMenu");
      
      gtk_box_pack_start (GTK_BOX (window->main_box), window->menubar, FALSE, FALSE, 0);
      gtk_widget_show (window->menubar);
    }
  
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (window->main_box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  label = gtk_label_new_with_mnemonic (_("F_ind:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  window->entry = gtk_entry_new ();
  g_signal_connect (window->entry, "activate", G_CALLBACK (entry_activate_cb), window);
  gtk_box_pack_start (GTK_BOX (hbox), window->entry, TRUE, TRUE, 0);
  gtk_widget_show (window->entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), window->entry);

  window->context = get_context_from_loader (window);
  
  window->defbox = gdict_defbox_new ();
  if (window->context)
    gdict_defbox_set_context (GDICT_DEFBOX (window->defbox), window->context);
  
  gtk_box_pack_start (GTK_BOX (window->main_box), window->defbox, TRUE, TRUE, 0);
  gtk_widget_show_all (window->defbox);

  window->status = gtk_statusbar_new ();
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (window->status), TRUE);
  gtk_box_pack_end (GTK_BOX (window->main_box), window->status, FALSE, FALSE, 0);
  gtk_widget_show (window->status);
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_window_class_init (GdictWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gdict_window_finalize;
  gobject_class->set_property = gdict_window_set_property;
  gobject_class->get_property = gdict_window_get_property;
  gobject_class->constructor = gdict_window_constructor;
  
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  							_("Source Loader"),
  							_("The GdictSourceLoader to be used to load dictionary sources"),
  							GDICT_TYPE_SOURCE_LOADER,
  							(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (gobject_class,
  				   PROP_WINDOW_ID,
  				   g_param_spec_uint ("window-id",
  				   		      _("Window ID"),
  				   		      _("The unique identificator for this window"),
  				   		      0,
  				   		      G_MAXUINT,
  				   		      0,
  				   		      G_PARAM_READABLE));

  gdict_window_signals[CREATED] =
    g_signal_new ("created",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictWindowClass, created),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GDICT_TYPE_WINDOW);
}

static void
gdict_window_init (GdictWindow *window)
{
  window->word = NULL;
  
  window->context = NULL;
  window->loader = NULL;
  
  window->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  window->window_id = (gulong) time (NULL);
}

GtkWidget *
gdict_window_new (GdictSourceLoader *loader)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  return g_object_new (GDICT_TYPE_WINDOW,
                       "source-loader", loader,
                       NULL);
}
