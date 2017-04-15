cssdir = $(htmldir)
imagedir = $(htmldir)/images

html_DATA = \
	html/ch01.html \
	html/ch01s01.html \
	html/ch01s02.html \
	html/ch01s03.html \
	html/ch02.html \
	html/ch02s01.html \
	html/ch02s02.html \
	html/ch03.html \
	html/ch03s01.html \
	html/ch03s02.html \
	html/ch03s03.html \
	html/ch03s04.html \
	html/ch04.html \
	html/ch04s01.html \
	html/ch04s02.html \
	html/ch04s03.html \
	html/ch04s04.html \
	html/ch04s05.html \
	html/ch04s06.html \
	html/index.html

css_DATA = html/xarchiver.css

image_DATA = \
	images/add_dialog.png \
	images/archive_comment_dialog.png \
	images/archive_properties.png \
	images/extract_dialog.png \
	images/multi_extract_dialog.png \
	images/new_dialog.png \
	images/password_dialog.png \
	images/pref_dialog.png

EXTRA_DIST = xarchiver.css xarchiver.docbook xarchiver.xsl \
	$(html_DATA) \
	$(css_DATA) \
	$(image_DATA)

html:
	xmlto -m xarchiver.xsl -o html/ xhtml xarchiver.docbook
	$(MAKE) $(AM_MAKEFLAGS) $(css_DATA)

pdf:
	jw -b pdf xarchiver.docbook
	mv xarchiver.pdf xarchiver-$(VERSION).pdf

doc-clean:
	rm -f html/*.html
	rm -f $(css_DATA)
	rm -f xarchiver-$(VERSION).pdf

$(html_DATA): xarchiver.docbook xarchiver.xsl
	rm -f html/*.html
	$(MAKE) $(AM_MAKEFLAGS) html

$(css_DATA): xarchiver.css
	cp xarchiver.css html

install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(docdir)" || exit 1
	-$(INSTALL_DATA) $(top_srcdir)/AUTHORS "$(DESTDIR)$(docdir)"
	$(INSTALL_DATA) $(top_srcdir)/COPYING "$(DESTDIR)$(docdir)"
	$(INSTALL_DATA) $(top_srcdir)/ChangeLog "$(DESTDIR)$(docdir)"
	-$(INSTALL_DATA) $(top_srcdir)/NEWS "$(DESTDIR)$(docdir)"
	$(INSTALL_DATA) $(top_srcdir)/README "$(DESTDIR)$(docdir)"
	-$(INSTALL_DATA) xarchiver-$(VERSION).pdf "$(DESTDIR)$(docdir)"

uninstall-local:
	rm -f "$(DESTDIR)$(docdir)/*"

.PHONY: doc-clean html pdf
