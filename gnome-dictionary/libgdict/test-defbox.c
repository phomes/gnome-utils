#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "gdict-defbox.h"
#include "gdict-client-context.h"

static void
do_search (GtkWidget *button,
	   gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  
  gdict_defbox_lookup (defbox, "GNOME");
}

int main (int argc, char *argv[])
{
  GdictContext *context;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *defbox;
  GtkWidget *button;

  gtk_init (&argc, &argv);
  
  context = gdict_client_context_new ("localhost", -1);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GdictDefbox");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show (window);
  
  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);
  
  defbox = gdict_defbox_new (context);
  gtk_box_pack_start (GTK_BOX (vbox), defbox, TRUE, TRUE, 0);
  gtk_widget_show (defbox);
  
  button = gtk_button_new_with_label ("Search 'GNOME'");
  g_signal_connect (button, "clicked", G_CALLBACK (do_search), defbox);
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_main ();
  
  return EXIT_SUCCESS;
}
