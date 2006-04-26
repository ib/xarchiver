noinst_LIBRARIES = libxarchiver.a

libxarchiver_a_SOURCES =            \
	libxarchiver.c    libxarchiver.h    \
	internals.c       internals.h       \
	support-gzip.c    support-gzip.h    \
	support-bzip2.c   support-bzip2.h    \
	support-gnu-tar.c support-gnu-tar.h    \
	support-zip.c	  support-zip.h    \
	support-rar.c     support-rar.h    \
	support.c         support.h         \
	archive.c         archive.h         \
	archive-types.c   archive-types.h

libxarchiver_a_CFLAGS = \
	@GLIB_CFLAGS@
libxarchiver_a_LIBADD =

libxarchiver_includedir= \
	$(includedir)/libxarchiver
