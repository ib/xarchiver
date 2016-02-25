SUBDIRS = 16x16 24x24 48x48 scalable

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
	  echo "Updating GTK icon cache."; \
	  $(gtk_update_icon_cache); \
	else \
	  echo "*** GTK icon cache not updated. Remember to run:"; \
	  echo "***"; \
	  echo "***   $(gtk_update_icon_cache)"; \
	  echo "***"; \
	fi

install-data-hook: update-icon-cache

uninstall-hook: update-icon-cache
