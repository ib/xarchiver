bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	socket.c socket.h \
	main.c main.h \
	string_utils.c string_utils.h \
	support.c support.h \
	archive.c archive.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	arj.c arj.h \
	deb.c deb.h \
	zip.c zip.h \
	rar.c rar.h \
	7zip.c 7zip.h \
	tar.c tar.h \
	bzip2.c bzip2.h \
	gzip.c gzip.h \
	rpm.c rpm.h \
	iso.c iso.h \
	lha.c lha.h \
	extract_dialog.c extract_dialog.h \
	add_dialog.c add_dialog.h \
	new_dialog.c new_dialog.h \
	pref_dialog.c pref_dialog.h

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
