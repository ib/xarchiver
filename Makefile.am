SUBDIRS = $(DOC) $(ICONS) pixmaps po src
DIST_SUBDIRS = doc icons pixmaps po src

desktopdir = $(datadir)/applications
desktop_in_files = xarchiver.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
	$(SED_V) $(SED) -i.tmp "/^Keywords[^_]/ s:$$:`sed -n "s:^Keywords_=\(.*\)$$:\1:p" $<`:; /^Keywords_/d" $@ && rm $@.tmp

SED_V = $(SED__v_$(V))
SED__v_ = $(SED__v_$(AM_DEFAULT_VERBOSITY))
SED__v_0 = @echo "  KWORD " $@;

plugindir = $(PLUGINDIR)
plugin_SCRIPTS = $(PLUGINSCRIPT)
EXTRA_SCRIPTS =

man1_MANS = doc/xarchiver.1

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
