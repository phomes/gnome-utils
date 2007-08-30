plugindir = $(libdir)/gnome-system-log/plugins

NULL =

if FALSE
libtest_la_LDFLAGS	= \
	$(PLUGIN_LIBTOOL_FLAGS) $(GNOME_UTILS_LIBS) $(GMODULE_LIBS)
plugin_LTLIBRARIES =	libtest.la
libtest_la_SOURCES	= test.c
endif

libgrablogs_la_LDFLAGS	= \
	$(PLUGIN_LIBTOOL_FLAGS) $(GNOME_UTILS_LIBS) $(GMODULE_LIBS)
libplainlog_la_LDFLAGS	= \
	$(PLUGIN_LIBTOOL_FLAGS) $(GNOME_UTILS_LIBS) $(GMODULE_LIBS)
libpipelog_la_LDFLAGS	= \
	$(PLUGIN_LIBTOOL_FLAGS) $(GNOME_UTILS_LIBS) $(GMODULE_LIBS)
libbaseview_la_LDFLAGS	= \
	$(PLUGIN_LIBTOOL_FLAGS) $(GNOME_UTILS_LIBS) $(GMODULE_LIBS)

plugin_LTLIBRARIES = \
	libgrablogs.la	\
	libplainlog.la	\
	libpipelog.la	\
	libbaseview.la

libgrablogs_la_SOURCES	= grablogs.c
libplainlog_la_SOURCES	= plainlog.c
libpipelog_la_SOURCES	= pipelog.c
libbaseview_la_SOURCES	= baseview.c

.PHONY: always

additionaldir  = $(plugindir)
additional_DATA  = \
	grablogs.conf	\
	pipelog.conf

EXTRA_DIST = \
	$(additional_DATA)

#
# This part allows people to build their own plugins in here.
# Yes, it's a mess.
#
SUFFIXES: .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_srcdir) $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(GNOME_UTILS_CFLAGS)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version $(GNOME_UTILS_LIBS)
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@.so* $@
	@rm -f .libs/libtmp$@.*


AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\"				\
	-I$(top_builddir)/logview				\
	-I$(top_srcdir)/logview					\
	$(GNOME_UTILS_CFLAGS)					\
	$(SUN_OS)						\
	$(NULL)


