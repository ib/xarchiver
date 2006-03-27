## Process this file with automake to produce Makefile.in

SUBDIRS = src libxarchiver po

pixmapsdir       = $(datadir)/pixmaps
pixmaps_DATA     = xarchiver.png

EXTRA_DIST = \
	autogen.sh \
	xarchiver.desktop \
	$(pixmaps_DATA)

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
