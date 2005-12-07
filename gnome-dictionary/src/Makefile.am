NULL =

INCLUDES = \
	-DDATADIR=\""$(datadir)"\" 			\
	-DUIDATADIR=\""$(datadir)/gnome-2.0/ui"\" 	\
	-DPKGDATADIR=\""$(datadir)/gnome-dictionary"\" 	\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

bin_PROGRAMS = gnome-dictionary

#libexec_PROGRAMS = gnome-dictionary-applet

gnome_dictionary_SOURCES = \
	gdict-about.c		\
	gdict-about.h		\
	gdict-app.c		\
	gdict-app.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	main.c			\
	$(NULL)
gnome_dictionary_CFLAGS = \
	-I$(top_srcdir)/gnome-dictionary/libgdict \
	$(GNOME_UTILS_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	$(LIBGNOMEPRINT_CFLAGS) \
	$(NULL)
gnome_dictionary_LDADD = \
	$(top_srcdir)/gnome-dictionary/libgdict/libgdict-1.0.la \
	$(GNOME_UTILS_LIBS) \
	$(LIBGLADE_LIBS) \
	$(LIBGNOMEPRINT_LIBS) \
	$(NULL)

# the applet container widget
noinst_LTLIBRARIES = libgtkalignedwindow.la
libgtkalignedwindow_la_SOURCES = \
	gtkalignedwindow.h \
	gtkalignedwindow.c \
	$(NULL)
libgtkalignedwindow_la_CFLAGS = $(GNOME_UTILS_CFLAGS)
libgtkalignedwindow_la_LIBADD = $(GNOME_UTILS_LIBS)

#gnome_dictionary_applet_SOURCES = \
#	gdict-about.c 		\
#	gdict-about.h		\
#	gdict-applet.c		\
#	gdict-applet.h		\
#	gdict-pref-dialog.c	\
#	gdict-pref-dialog.h	\
#	$(NULL)
#gnome_dictionary_applet_CFLAGS = \
#	-I$(top_srcdir)/gnome-dictionary/libgdict \
#	$(GNOME_UTILS_CFLAGS) \
#	$(LIBGLADE_CFLAGS) \
#	$(LIBGNOMEPRINT_CFLAGS) \
#	$(APPLET_CFLAGS) \
#	$(NULL)
#gnome_dictionary_applet_LDADD = \
#	$(top_srcdir)/gnome-dictionary/libgdict/libgdict-1.0.la \
#	$(top_srcdir)/gnome-dictionary/src/libgtkalignedwindow.la \
#	$(GNOME_UTILS_LIBS) \
#	$(LIBGLADE_LIBS) \
#	$(LIBGNOMEPRINT_LIBS) \
#	$(APPLET_LIBS) \
#	$(NULL)

