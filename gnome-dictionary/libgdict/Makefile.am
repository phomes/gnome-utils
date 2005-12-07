## Process this file with automake to produce Makefile.in

NULL =

INCLUDES = -DG_LOG_DOMAIN=\"Gdict\" 				\
	-DDATADIR=\""$(datadir)"\" 				\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DPREFIX=\""$(prefix)"\" 				\
	-DGNOMELOCALEDIR=\""$(gnomeutilslocaledir)"\" 		\
	-DGDICTSOURCESDIR=\""$(datadir)/gdict-1.0/sources"\" 	\
	-DGDICT_ENABLE_INTERNALS=1 				\
	$(DISABLE_DEPRECATED_CFLAGS) 				\
	$(WARN_CFLAGS) 						\
	$(NULL)

noinst_LTLIBRARIES = libgdict-1.0.la

noinst_PROGRAMS = test-client test-entry test-defbox test-source

# GNOME Dictionary shared library
libgdict_1_0_la_SOURCES =	\
	gdict-context.c		\
	gdict-context.h		\
	gdict-context-private.h	\
	gdict-client-context.c	\
	gdict-client-context.h	\
	gdict-defbox.c 		\
	gdict-defbox.h 		\
	gdict-entry.c		\
	gdict-entry.h		\
	gdict-enum-types.c	\
	gdict-enum-types.h	\
	gdict-marshal.c		\
	gdict-marshal.h		\
	gdict-source.c		\
	gdict-source.h		\
	gdict-source-loader.c	\
	gdict-source-loader.h	\
	gdict-utils.c		\
	gdict-utils.h		\
	$(NULL)

libgdict_1_0_la_CFLAGS = $(LIBGDICT_CFLAGS)
libgdict_1_0_la_LDFLAGS = \
	-version-info $(LIBGDICT_LT_VERSION) \
	-export-dynamic \
	-no-undefined
libgdict_1_0_la_LIBADD = $(LIBGDICT_LIBS)

libgdict_includedir = $(includedir)/gdict-1.0/gdict
libgdict_include_HEADERS = 	\
	gdict-context.h		\
	gdict-client-context.h	\
	gdict-defbox.h 		\
	gdict-entry.h		\
	gdict-enum-types.h	\
	gdict-source.h		\
	gdict-source-loader.h	\
	gdict-utils.h		\
	gdict.h			\
	$(NULL)

BUILT_SOURCES = 		\
	gdict-enum-types.h 	\
	gdict-enum-types.c 	\
	gdict-marshal.h		\
	gdict-marshal.c		\
	$(NULL)

$(libgdict_1_0_la_OBJECTS): $(BUILT_SOURCES)

gdict-marshal.h: gdict-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gdict_marshal > $@

gdict-marshal.c: gdict-marshal.list $(GLIB_GENMARSHAL)
	@echo "#include \"gdict-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gdict_marshal > $@

libgdict_enum_headers = \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-context.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-client-context.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-defbox.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-entry.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-source-loader.h \
	$(top_srcdir)/gnome-dictionary/libgdict/gdict-utils.h \
	$(NULL)

gdict-enum-types.h: $(libgdict_enum_headers)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef __GDICT_ENUM_TYPES_H__\n" \
		--fhead "#define __GDICT_ENUM_TYPES_H__\n\n" \
		--fhead "G_BEGIN_DECLS\n\n" \
		--ftail "G_END_DECLS\n\n" \
		--ftail "#endif\n" \
		--fprod "/* --- @filename@ --- */\n" \
		--fprod "#include \"@filename@\"\n" \
		--eprod "#define GDICT_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
		--eprod "GType @enum_name@_get_type (void);\n\n" \
	$(libgdict_enum_headers) > xgen-gth \
	&& cp xgen-gth $(@F) \
	&& rm -f xgen-gth

gdict-enum-types.c: $(libgdict_enum_headers)
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fhead "#include \"gdict-enum-types.h\"\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
		--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\n" \
		--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
		--vtail "  static GType type = 0;\n\n" \
		--vtail "  if (!type)\n" \
		--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
		--vtail "  return type;\n}\n\n" \
	$(libgdict_enum_headers) > xgen-gtbec \
	&& cp xgen-gtbec $(@F) \
	&& rm -f xgen-gtbec

test_client_SOURCES = test-client.c
test_client_CFLAGS = -O2 -ggdb $(LIBGDICT_CFLAGS)
test_client_LDADD = libgdict-1.0.la $(LIBGDICT_LIBS)

test_entry_SOURCES = test-entry.c
test_entry_CFLAGS = -O2 -ggdb $(LIBGDICT_CFLAGS)
test_entry_LDADD = libgdict-1.0.la $(LIBGDICT_LIBS)

test_defbox_SOURCES = test-defbox.c
test_defbox_CFLAGS = -O2 -ggdb $(LIBGDICT_CFLAGS)
test_defbox_LDADD = libgdict-1.0.la $(LIBGDICT_LIBS)

test_source_SOURCES = test-source.c
test_source_CFLAGS = -O2 -ggdb $(LIBGDICT_CFLAGS)
test_source_LDADD = libgdict-1.0.la $(LIBGDICT_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdict-1.0.pc

EXTRA_DIST = 				\
	gdict-marshal.list		\
	gdict-1.0.pc.in			\
	gdict-1.0-uninstalled.pc.in	\
	$(NULL)

CLEANFILES = 			\
	$(BUILT_SOURCES) 	\
	$(NULL)

DISTCLEANFILES = 		\
	$(CLEANFILES)		\
	gdict-version.h 	\
	gdict-1.0.pc		\
	gdict-1.0-uninstalled.pc \
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
	
