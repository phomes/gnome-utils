#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gdict-context.h"
#include "gdict-client-context.h"
#include "gdict-entry.h"

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GdictContext *context;
  
  gtk_init (&argc, &argv);
  
  context = gdict_client_context_new ("localhost", 2628);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GdictEntry");
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);
  
  entry = gdict_entry_new (context);
  g_object_unref (context); /* let the entry take the ownership */
  g_object_set (entry, "database", "vera", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);
  
  label = gtk_label_new ("GdictEntry test: type something inside the entry "
                         "and watch for completion");
  gtk_box_pack_end (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);
  
  gtk_widget_show (window);
  
  gtk_main ();
  
  return 0;
}
