#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "gdict-context.h"
#include "gdict-client-context.h"

static GMainLoop *main_loop = NULL;

static void
connected_cb (GdictClientContext *context,
	      gpointer            user_data)
{
  g_print ("* Connected to '%s (port: %d)'\n",
  	   gdict_client_context_get_hostname (context),
  	   gdict_client_context_get_port (context));
}

static void
disconnected_cb (GdictClientContext *context,
	         gpointer            user_data)
{
  g_print ("* Disconnected from '%s'\n",
  	   gdict_client_context_get_hostname (context));
  
  g_main_loop_quit (main_loop);
}

static void
database_found_cb (GdictContext  *context,
		   GdictDatabase *database,
		   gpointer       user_data)
{
  g_print ("* Database found:\n"
           "*   Name: '%s'\n"
           "*   Description: '%s'\n",
           gdict_database_get_name (database),
           gdict_database_get_full_name (database));
}

int main (int argc, char *argv[])
{
  GdictContext *context;
  GError *db_error = NULL;

  g_type_init ();
  
  main_loop = g_main_loop_new (NULL, FALSE);
  
  context = gdict_client_context_new ("localhost", 2628);
  g_signal_connect (context, "connected",
                    G_CALLBACK (connected_cb), NULL);
  g_signal_connect (context, "disconnected",
                    G_CALLBACK (disconnected_cb), NULL);
  g_signal_connect (context, "database-found",
		    G_CALLBACK (database_found_cb), NULL);
                    
  gdict_context_lookup_databases (context, &db_error);
  if (db_error)
    {
      g_warning ("Unable to look up databases: %s\n",
                 db_error->message);
      
      g_error_free (db_error);
      g_object_unref (context);
      
      return EXIT_FAILURE;
    }
  
  g_main_loop_run (main_loop);
  
  g_object_unref (context);
  
  return EXIT_SUCCESS;
}
