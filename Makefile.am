SUBDIRS = $(DOC) $(ICONS) pixmaps po src
DIST_SUBDIRS = doc icons pixmaps po src

desktopdir = $(datadir)/applications
desktop_in_files = xarchiver.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

plugindir = $(PLUGINDIR)
plugin_SCRIPTS = $(PLUGINSCRIPT)
EXTRA_SCRIPTS =

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(desktop_in_files) \
	$(plugin_SCRIPTS)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA)
