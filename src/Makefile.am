bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	main-window.c main-window.h \
	property-dialog.c property-dialog.h \
	extract-dialog.c extract-dialog.h \
	new-dialog.c new-dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"
 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@
	-lz

INCLUDES = \
	-I${top_srcdir}
