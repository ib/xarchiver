bin_PROGRAMS = xarchiver

SUBDIRS = xdgmime

xarchiver_SOURCES = \
	socket.c socket.h \
	sexy-icon-entry.c sexy-icon-entry.h \
	main.c main.h \
	string_utils.c string_utils.h \
	mime.c mime.h \
	utf8-fnmatch.c utf8-fnmatch.h \
	support.c support.h \
	archive.c archive.h \
	interface.c interface.h \
	window.c window.h \
	arj.c arj.h \
	deb.c deb.h \
	zip.c zip.h \
	rar.c rar.h \
	7zip.c 7zip.h \
	tar.c tar.h \
	bzip2.c bzip2.h \
	gzip.c gzip.h \
	rpm.c rpm.h \
	lha.c lha.h \
	extract_dialog.c extract_dialog.h \
	add_dialog.c add_dialog.h \
	new_dialog.c new_dialog.h \
	open-with-dlg.c open-with-dlg.h \
	pref_dialog.c pref_dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPIXMAPSDIR=\"$(XA_PIXMAPSDIR)\"

xarchiver_LDADD = \
	$(top_builddir)/src/xdgmime/libxdgmime.la \
	@GTK_LIBS@ \
	@GLIB_LIBS@

INCLUDES = \
	-I${top_srcdir}
