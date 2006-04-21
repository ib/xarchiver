## Process this file with automake to produce Makefile.in

SUBDIRS = libxarchiver src po

pixmapsdir       = $(datadir)/pixmaps

EXTRA_DIST = \
	autogen.sh \
	xarchiver.desktop

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

Applicationsdir = $(datadir)/applications
Applications_DATA = xarchiver.desktop

ACLOCAL_AMFLAGS = -I m4
