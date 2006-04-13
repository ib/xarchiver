bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	archive-chooser-dialog.c archive-chooser-dialog.h \
	main-window.c main-window.h \
	property-dialog.c property-dialog.h 

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@GTHREAD_CFLAGS@ \
	-DDATADIR=\"$(datadir)\"

 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@GTHREAD_LIBS@ \
	-lz

INCLUDES = \
	-I${top_srcdir}
