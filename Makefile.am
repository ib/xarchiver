## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS =							\
	1.8								\
	dist-bzip2

SUBDIRS = $(ICONS) pixmaps po src $(DOC)
DIST_SUBDIRS = icons pixmaps po src doc

wrapperdir = $(WRAPPERDIR)
wrapper_SCRIPTS = $(WRAPPERSCRIPTS)
EXTRA_SCRIPTS = xarchiver.tap

desktopdir = $(datadir)/applications
desktop_in_files = xarchiver.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	xarchiver.tap \
	$(desktop_in_files) \
	$(pixmaps_DATA) \
	$(wrapper_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA)
