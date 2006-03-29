bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	main-window.c main-window.h 

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@

 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	-lz

INCLUDES = \
	-I${top_srcdir}
