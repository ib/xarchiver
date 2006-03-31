#ifndef __XARCHIVER_RAR_SUPPORT_H__
#define __XARCHIVER_RAR_SUPPORT_H__


XArchiveSupport *xarchive_rar_support_new();
gboolean xarchiver_parse_rar_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gboolean jump_header;
gboolean odd_line;
#endif /* __XARCHIVER_RAR_SUPPORT_H__ */
