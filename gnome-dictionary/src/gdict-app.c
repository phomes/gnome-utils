/* gdict-app.c - main application class
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

#include <gtk/gtk.h>
#include <glib/goption.h>
#include <glib/gi18n.h>
#include <libgnome/libgnome.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprintui/gnome-print-dialog.h>

#include "gdict.h"

#include "gdict-about.h"
#include "gdict-app.h"

#define GDICT_WINDOW_MIN_WIDTH	400
#define GDICT_WINDOW_MIN_HEIGHT	300

static GdictApp *singleton = NULL;

/***************
 * GdictWindow *
 ***************/

struct _GdictWindowClass
{
  GtkWindowClass parent_class;
};

enum
{
  PROP_0,
  
  PROP_SOURCE_LOADER,
  PROP_WINDOW_ID,
  PROP_WORD
};



G_DEFINE_TYPE (GdictWindow, gdict_window, GTK_TYPE_WINDOW);


static void
gdict_window_finalize (GObject *object)
{
  GdictWindow *window = GDICT_WINDOW (object);
  
  g_object_unref (window->loader);
  
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
gdict_window_cmd_file_print (GtkAction   *action,
			     GdictWindow *window)
{  
  g_assert (GDICT_IS_WINDOW (window));
  
  if (!window->print_dialog)
    {
      GtkWidget *dialog;
      
      if (!window->print_job)
      window->print_dialog = dialog;
    }
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
  { "Escape", NULL, "", "Escape", "", G_CALLBACK (gdict_window_cmd_escape) },
  { "Slash", GTK_STOCK_FIND, NULL, "slash", NULL, G_CALLBACK (gdict_window_cmd_edit_find) },
};

static GdictContext *
get_context_from_loader (GdictSourceLoader *loader)
{
  GdictSource *source;
  GdictContext *retval;
  
  source = gdict_source_loader_get_source (loader, _("Default"));
  if (!source)
    {
      g_warning ("Unable to find a dictionary source");
      return NULL;
    }
  
  retval = gdict_source_get_context (source);
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
  
  label = gtk_label_new (_("Find:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  window->entry = gtk_entry_new ();
  g_signal_connect (window->entry, "activate", G_CALLBACK (entry_activate_cb), window);
  gtk_box_pack_start (GTK_BOX (hbox), window->entry, TRUE, TRUE, 0);
  gtk_widget_show (window->entry);

  window->context = get_context_from_loader (window->loader);
  
  window->defbox = gdict_defbox_new ();
  if (window->context)
    gdict_defbox_set_context (GDICT_DEFBOX (window->defbox), window->context);
  
  gtk_box_pack_start (GTK_BOX (window->main_box), window->defbox, TRUE, TRUE, 0);
  gtk_widget_show_all (window->defbox);
  
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
}

static void
gdict_window_init (GdictWindow *window)
{
  if (!singleton)
    {
      g_error ("You can instantiate a GdictWindow without calling gdict_init() first");
    }
    
  window->app = singleton;
  
  window->word = NULL;
  
  window->context = NULL;
  window->loader = NULL;
  
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


/************
 * GdictApp *
 ************/


G_DEFINE_TYPE (GdictApp, gdict_app, G_TYPE_OBJECT);


static void
gdict_app_finalize (GObject *object)
{
  GdictApp *app = GDICT_APP (object);
  
  if (app->loader)
    g_object_unref (app->loader);
  
  app->current_window = NULL;
  
  g_slist_foreach (app->windows,
  		   (GFunc) gtk_widget_destroy,
  		   NULL);
  g_slist_free (app->windows);
  
  G_OBJECT_CLASS (gdict_app_parent_class)->finalize (object);
}

static void
gdict_app_class_init (GdictAppClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_app_finalize;

#if 0  
  g_signal_new ("window-closed",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GdictAppClass, window_closed),
		NULL, NULL,
		gdict_marshal_VOID__OBJECT,
		G_TYPE_NONE, 1,
		GDICT_TYPE_WINDOW);
#endif
}

static void
gdict_app_init (GdictApp *app)
{

}

static gboolean
save_yourself_cb (GnomeClient        *client,
		  gint                phase,
		  GnomeSaveStyle      save_style,
		  gint                shutdown,
		  GnomeInteractStyle  interact_style,
		  gint                fast,
		  gpointer            user_data)
{
  return TRUE;
}

static void
window_destroy_cb (GtkWidget *widget,
		   gpointer   user_data)
{
  GdictWindow *window = GDICT_WINDOW (widget);
  GSList *l;
  
  g_assert (GDICT_IS_APP (singleton));
  
  for (l = singleton->windows; l != NULL; l = l->next)
    {
      GdictWindow *w = GDICT_WINDOW (l->data);
      
      if (w->window_id == window->window_id)
        {
          singleton->windows = g_slist_remove_link (singleton->windows, l);
          g_slist_free (l);

          break;
        }
    }

  if (g_slist_length (singleton->windows) == 0)
    gtk_main_quit ();
}

void
gdict_init (int *argc, char ***argv)
{
  GdictApp *app;
  GError *error;
  GOptionContext *context;
  gchar *loader_path;

  g_type_init ();

  g_assert (singleton == NULL);  
  
  singleton = GDICT_APP (g_object_new (GDICT_TYPE_APP, NULL));
  g_assert (GDICT_IS_APP (singleton));
  
  singleton->program = gnome_program_init ("gnome-dictionary",
					   VERSION,
					   LIBGNOMEUI_MODULE,
					   *argc, *argv,
					   GNOME_PARAM_APP_DATADIR, DATADIR,
					   NULL);
  g_set_application_name (_("Dictionary"));
  gtk_window_set_default_icon_name ("gdict");
  
  singleton->client = gnome_master_client ();
  if (singleton->client)
    {
      g_signal_connect (singleton->client, "save-yourself",
                        G_CALLBACK (save_yourself_cb),
                        NULL);
    }
  
  singleton->loader = gdict_source_loader_new ();
  loader_path = g_build_filename (g_get_home_dir (),
                                  ".gnome2",
                                  "gnome-dictionary",
                                  "gdict-1.0",
                                  NULL);
  gdict_source_loader_add_search_path (singleton->loader, loader_path);
  g_free (loader_path);
  
  context = g_option_context_new (_(" - Dictionary and spelling tool"));
  g_option_context_set_ignore_unknown_options (context, TRUE);
  
  error = NULL;
  g_option_context_parse (context, argc, argv, &error);
}

void
gdict_main (void)
{
  GtkWidget *window;
  
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_app_init()\n");
      return;
    }
  
  window = gdict_window_new (singleton->loader);
  g_signal_connect (window, "destroy", G_CALLBACK (window_destroy_cb), NULL);
  
  singleton->current_window = GDICT_WINDOW (window);
  singleton->windows = g_slist_prepend (singleton->windows, window);

  gtk_widget_show (window);
  
  gtk_main ();
}

void
gdict_cleanup (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_app_init()\n");
      return;
    }

  g_object_unref (singleton);
}
