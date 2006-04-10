#ifndef __XARCHIVER_TAR_SUPPORT_H__
#define __XARCHIVER_TAR_SUPPORT_H__


XArchiveSupport *xarchive_tar_support_new();
gboolean xarchiver_parse_tar_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

#endif /* __XARCHIVER_TAR_SUPPORT_H__ */
