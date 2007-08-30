## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=logview

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/logview

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/logview/*.h
CFILE_GLOB=$(top_srcdir)/logview/*.c

# Header files to ignore when scanning.
IGNORE_HFILES =			\
	about.h			\
	calendar.h		\
	log_repaint.h		\
	logview.h		\
	monitor.h		\
	userprefs.h		\
	logview-findbar.h	\
	loglist.h		\
	logview-module.h	\
	logview-plugin.h	\
	logview-plugin-manager.h	\
	logview-plugin-list.h

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
INCLUDES=				\
	$(CFLAGS)			\
	$(CPPFLAGS)			\
	$(GNOME_UTILS_CFLAGS)		\
	-I$(top_srcdir)/logview		\
	-I$(top_builddir)/logview

GTKDOC_LIBS=			\
	$(LDFLAGS)		\
	$(GNOME_UTILS_LIBS)	\
	$(top_builddir)/logview/libgnome-system-log.la

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST +=
