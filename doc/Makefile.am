DOCDIR = $(DESTDIR)$(datadir)/doc/@PACKAGE@
HTML_FILES = html/*.html xarchiver.css
IMAGE_FILES = images/*.png
EXTRA_DIST = xarchiver.css xarchiver.xsl \
		xarchiver.docbook $(HTML_FILES) $(IMAGE_FILES)

pdf:
	jw -b pdf xarchiver.docbook
	mv xarchiver.pdf geany-$(VERSION).pdf

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
	for file in `ls $(HTML_FILES)`; do \
	  if [ -f $$file ]; then \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $(srcdir)/$$file $(DOCDIR)/html/$$basefile; \
	  fi \
	done
	for file in `ls $(IMAGE_FILES)`; do \
	  if [ -f $$file ]; then \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $(srcdir)/$$file $(DOCDIR)/html/images/$$basefile; \
	  fi \
	done
	$(INSTALL_DATA) ../README $(DOCDIR)
	$(INSTALL_DATA) ../ChangeLog $(DOCDIR)
	$(INSTALL_DATA) ../NEWS $(DOCDIR)
	$(INSTALL_DATA) ../COPYING $(DOCDIR)
	$(INSTALL_DATA) ../AUTHORS $(DOCDIR)
	$(INSTALL_DATA) ../TODO $(DOCDIR)

