#ifndef __LIBXARCHIVER_INTERNALS_H__
#define __LIBXARCHIVER_INTERNALS_H__

static int n_cwd = 0;
gchar *escape_filename ( gchar *string );
GString *concatenatefilenames ( GSList *list );
#endif
