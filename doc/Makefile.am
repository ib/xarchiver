DOCDIR = $(DESTDIR)$(datadir)/doc/@PACKAGE@
HTML_FILES = \
	html/ch01.html \
	html/ch01s02.html \
	html/ch01s03.html \
	html/ch01s04.html \
	html/ch02.html \
	html/ch02s02.html \
	html/ch02s03.html \
	html/ch03.html \
	html/ch03s02.html \
	html/ch03s03.html \
	html/ch03s04.html \
	html/ch03s05.html \
	html/ch03s06.html \
	html/ch03s07.html \
	html/ch03s08.html \
	html/ch03s09.html \
	html/ch04.html \
	html/ch04s02.html \
	html/ch04s03.html \
	html/ch04s04.html \
	html/ch04s05.html \
	html/ch04s06.html \
	html/index.html \
	xarchiver.css
IMAGE_FILES = \
	images/add_dialog.png \
	images/archive_properties.png \
	images/extract_dialog.png \
	images/new_dialog.png \
	images/archive_comment_dialog.png \
	images/multi_extract_dialog.png \
	images/password_dialog.png \
	images/pref_dialog.png

EXTRA_DIST = xarchiver.css xarchiver.xsl \
		xarchiver.docbook $(HTML_FILES) $(IMAGE_FILES)

pdf:
	jw -b pdf xarchiver.docbook
	mv xarchiver.pdf xarchiver-$(VERSION).pdf

htmldoc: xarchiver.docbook
	xmlto -m xarchiver.xsl -o html/ xhtml xarchiver.docbook; \
	test -f html/xarchiver.css || cp xarchiver.css html/;

doc-clean:
	rm -rf html/*.html
	rm -f html/xarchiver.css
	rm -f *.pdf

uninstall-local:
	rm -rf $(DOCDIR);

install-data-local:
	$(mkinstalldirs) $(DOCDIR)/html/images
	for file in $(HTML_FILES); do \
	  if [ -f $$file ]; then \
	    basefile=`echo $(srcdir)/$$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $(srcdir)/$$file $(DOCDIR)/html/$$basefile; \
	  fi \
	done
	for file in $(IMAGE_FILES); do \
	  if [ -f $(srcdir)/$$file ]; then \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $(srcdir)/$$file $(DOCDIR)/html/images/$$basefile; \
	  fi \
	done
	$(INSTALL_DATA) $(top_srcdir)/README $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/ChangeLog $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/NEWS $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/AUTHORS $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/TODO $(DOCDIR)

