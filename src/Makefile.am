bin_PROGRAMS = xarchiver


xarchiver_SOURCES = \
	xarchivericon.h \
	main.c main.h \
	main-window.c main-window.h \
	property-dialog.c property-dialog.h \
	archive-chooser-dialog.c archive-chooser-dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DDATADIR=\"$(datadir)\"
 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@
	-lz

xarchivericon.h: xarchivericon.png
	gdk-pixbuf-csource --raw --name=xarchivericon \
	xarchivericon.png > xarchivericon.h

INCLUDES = \
	-I${top_srcdir}

EXTRA_DIST = xarchivericon.png
