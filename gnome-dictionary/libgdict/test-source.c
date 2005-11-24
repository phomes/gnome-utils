#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "gdict-context.h"
#include "gdict-client-context.h"
#include "gdict-source.h"
#include "gdict-source-loader.h"

static void
dump_source_info (GdictSource *source)
{
  GdictSourceTransport transport;
  GdictContext *context;
  
  g_assert (GDICT_IS_SOURCE (source));
  
  transport = gdict_source_get_transport (source);
  context = gdict_source_get_context (source);
 
  g_print ("(in %s)\n", G_STRFUNC);
  
  g_print ("GdictSource source (%p) =\n"
           "{\n"
           "  .name        = '%s',\n"
           "  .description = '%s',\n"
           "  .transport   = '%d',\n",
           source,
           gdict_source_get_name (source),
           gdict_source_get_description (source),
           transport);
  
  switch (transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      g_print ("  .hostname    = '%s',\n"
               "  .port        = '%d',\n",
               gdict_client_context_get_hostname (GDICT_CLIENT_CONTEXT (context)),
               gdict_client_context_get_port (GDICT_CLIENT_CONTEXT (context)));
      break;
    default:
      g_assert_not_reached ();
    }
  
  g_print ("}\n");
}

static void
source_loaded_cb (GdictSourceLoader *loader,
		  GdictSource       *source,
		  gpointer           user_data)
{
  g_print ("(in %s)\n", G_STRFUNC);

  dump_source_info (source);
}

int main (int argc, char *argv[])
{
  GdictSourceLoader *loader;
  GMainLoop *main_loop;
  GSList *sources, *l;
  
  g_type_init ();
  
  main_loop = g_main_loop_new (NULL, FALSE);
  
  loader = gdict_source_loader_new ();
  g_signal_connect (loader, "source-loaded", G_CALLBACK (source_loaded_cb), NULL);
  
  sources = gdict_source_loader_get_sources (loader);
  for (l = sources; l; l = l->next)
    {
      GdictSource *s = GDICT_SOURCE (l->data);
      
      g_assert (GDICT_IS_SOURCE (s));
      
      dump_source_info (s);
    }
  
  g_main_loop_run (main_loop);
  
  g_object_unref (loader);
  
  return EXIT_SUCCESS;
}
