uidir   = $(datadir)/baobab
ui_DATA = baobab-main-window.ui \
	  baobab-dialog-scan-props.ui

baobabappdir	   = $(datadir)/applications
baobabapp_in_files = baobab.desktop.in
baobabapp_DATA     = $(baobabapp_in_files:.desktop.in=.desktop)

$(baobabapp_in_files): $(baobabapp_in_files:.desktop.in=.desktop.in.in)
	sed -e "s|\@VERSION\@|@VERSION@|" $< > $@

@INTLTOOL_DESKTOP_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = baobab.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

gsettingsschema_DATA = org.gnome.baobab.gschema
org.gnome.baobab.gschema: baobab.schemas
	gsettings-schema-convert --force --gconf --xml --schema-id $(@:.gschema=) $< -o $@
# --allow-any-name should be removed once the move from gconf is complete
install-data-hook:
	$(gsettingsupdateschemacache) --allow-any-name $(gsettingsschemadir)

man_MANS = baobab.1

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA) ; \
	fi
else
install-data-local:
endif

EXTRA_DIST = 				\
	$(ui_DATA)			\
	baobab.desktop.in.in		\
	$(schemas_in_files)		\
	$(man_MANS)

CLEANFILES =	 			\
	$(baobabapp_in_files)		\
	$(baobabapp_DATA)		\
	$(schemas_DATA)			\
	$(gsettingsschema_DATA)
