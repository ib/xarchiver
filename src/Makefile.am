bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	support.c support.h \
	archive.c archive.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	arj.c arj.h \
	zip.c zip.h \
	rar.c rar.h \
	7zip.c 7zip.h \
	tar.c tar.h \
	bzip2.c bzip2.h \
	gzip.c gzip.h \
	rpm.c rpm.h
#        iso.c iso.h 
#	extract-dialog.c extract-dialog.h \
#	new-dialog.c new-dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"
 
xarchiver_LDADD = \
	@GTK_LIBS@ \
	@GLIB_LIBS@

INCLUDES = \
	-I${top_srcdir}
