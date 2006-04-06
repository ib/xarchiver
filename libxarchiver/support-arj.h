#ifndef __XARCHIVER_ARJ_SUPPORT_H__
#define __XARCHIVER_ARJ_SUPPORT_H__


XArchiveSupport *xarchive_arj_support_new();
gboolean xarchiver_parse_arj_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gboolean jump_header;
gboolean odd_line;

#endif /* __XARCHIVER_ARJ_SUPPORT_H__ */
