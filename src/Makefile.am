bin_PROGRAMS = xarchiver

SUBDIRS = xdgmime

xarchiver_SOURCES = \
	7zip.c \
	7zip.h \
	add_dialog.c \
	add_dialog.h \
	archive.c \
	archive.h \
	arj.c \
	arj.h \
	bzip2.c \
	bzip2.h \
	deb.c \
	deb.h \
	extract_dialog.c \
	extract_dialog.h \
	gzip.c \
	gzip.h \
	interface.c \
	interface.h \
	lha.c \
	lha.h \
	main.c \
	main.h \
	mime.c \
	mime.h \
	new_dialog.c \
	new_dialog.h \
	open-with-dlg.c \
	open-with-dlg.h \
	pref_dialog.c \
	pref_dialog.h \
	rar.c \
	rar.h \
	rpm.c \
	rpm.h \
	socket.c \
	socket.h \
	string_utils.c \
	string_utils.h \
	support.h \
	tar.c \
	tar.h \
	utf8-fnmatch.c \
	utf8-fnmatch.h \
	window.c \
	window.h \
	zip.c \
	zip.h

if GTK_V2_16
else
xarchiver_SOURCES += sexy-icon-entry.c sexy-icon-entry.h
endif

xarchiver_CFLAGS = \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	@GTK_CFLAGS@ \
	-DHTMLDIR=\"$(htmldir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPIXMAPSDIR=\"$(PIXMAPDIR)\"

xarchiver_LDADD = \
	xdgmime/libxdgmime.la \
	@GTK_LIBS@
