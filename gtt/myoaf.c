

#include <gtk/gtk.h>
#include <liboaf/liboaf.h>

#include "myoaf.h"

void
edit_calendar(GtkWidget *w, gpointer data)
{
	OAF_ServerInfoList *sl;
	char * idl, *q;
	CORBA_Environment ev;
	CORBA_ORB   oaf_orb;
	CORBA_Object *cal;

	// oaf_orb = oaf_init (int argc, char **argv);

	

printf ("duude oaf is init=%d\n", oaf_is_initialized());

	oaf_orb = oaf_orb_get();


	idl = "IDL:GNOME/Calendar/Repository:1.0";

	/* this is the evolution calandar but it crashes */
	idl = "IDL:BonoboControl/calendar-control:1.0";

	idl = "IDL:GNOME/Calendar/RepositoryLocator:1.0";
	q = g_strconcat ("repo_ids.has ('", idl, "')", NULL);
	sl = oaf_query (q, NULL, NULL);
	cal = oaf_activate (q, NULL, 0, NULL, NULL);
printf ("duude query = %p item=%p\n", sl, cal);
}


