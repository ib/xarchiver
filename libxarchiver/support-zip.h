#ifndef __XARCHIVER_ZIP_SUPPORT_H__
#define __XARCHIVER_ZIP_SUPPORT_H__


XArchiveSupport *xarchive_zip_support_new();
gboolean
xarchiver_parse_zip_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

#endif /* __XARCHIVER_ZIP_SUPPORT_H__ */
