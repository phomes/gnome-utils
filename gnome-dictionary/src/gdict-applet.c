#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-help.h>

#include "gdict-context.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"

#include "gdict-applet.h"
#include "gdict-about.h"
#include "gdict-pref-dialog.h"

#define GDICT_APPLET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_APPLET, GdictAppletClass))
#define GDICT_APPLET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_APPLET, GdictAppletClass))

struct _GdictAppletClass
{
  PanelAppletClass parent_class;
};

#define GDICT_APPLET_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_APPLET, GdictAppletPrivate))
struct _GdictAppletPrivate
{
  
};

G_DEFINE_TYPE (GdictApplet, gdict_applet, PANEL_TYPE_APPLET);



static void
gdict_applet_finalize (GObject *object)
{

}

static void
gdict_applet_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{

}

static void
gdict_applet_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{

}

static void
gdict_applet_class_init (GdictAppletClass *klass)
{

}

static void
gdict_applet_init (GdictApplet *gdict_applet)
{

}
