## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	arj.c arj.h \
	zip.c zip.h \
	rar.c rar.h \
	rpm.c rpm.h \
	bzip2.c bzip2.h \
	gzip.c gzip.h \
	tar.c tar.h \
	iso.c iso.h \
	7zip.c 7zip.h
 
xarchiver_LDADD = @PACKAGE_LIBS@

