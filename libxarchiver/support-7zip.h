#ifndef __XARCHIVER_7ZIP_SUPPORT_H__
#define __XARCHIVER_7ZIP_SUPPORT_H__


XArchiveSupport *xarchive_7zip_support_new();
gboolean xarchiver_parse_7zip_output (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean jump_header;
#endif /* __XARCHIVER_7ZIP_SUPPORT_H__ */
