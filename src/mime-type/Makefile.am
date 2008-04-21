noinst_LTLIBRARIES = libmimetype.la

libmimetype_la_SOURCES = \
	mime-action.c \
	mime-action.h \
	mime-cache.c \
	mime-cache.h \
	mime-type.c \
	mime-type.h

INCLUDES = \
	-I${top_srcdir} -I. -I..

libmimetype_la_CFLAGS = \
	@GLIB_CFLAGS@

libmimetype_la_LIBADD = \
	@GLIB_LIBS@
