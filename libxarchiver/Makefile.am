noinst_LIBRARIES = libxarchiver.a

libxarchiver_a_SOURCES =            \
	libxarchiver.c    libxarchiver.h    \
	internals.c       internals.h       \
	support-gzip.c    support-gzip.h    \
	support-gnu-tar.c support-gnu-tar.h    \
	support.c         support.h         \
	archive.c         archive.h         \
	archive-types.c   archive-types.h

libxarchiver_a_CFLAGS = \
	@GLIB_CFLAGS@ \
	@GTHREAD_CFLAGS@

libxarchiver_a_LIBADD =

libxarchiver_includedir= \
	$(includedir)/libxarchiver
