#ifndef __LOGVIEW_DEBUG_H__
#define __LOGVIEW_DEBUG_H__
#include <glib.h>

/**
 * LogviewLevelFlags:
 * @LOGVIEW_INFO: Normal information will be displayed in this level.
 * @LOGVIEW_VERBOSE: Like %LOGVIEW_INFO, more information like file name,
 * function name and line number will be displayed in this level.
 * @LOGVIEW_ERR: Cover all above levels, displays verbose information
 * for debugging usage.
 * 
 * Used to specify the level of the log of gnome-system-log.
 **/
typedef enum {
	LOGVIEW_INFO		= 1 << 0,
	LOGVIEW_VERBOSE		= 1 << 1,
	LOGVIEW_ERR		= 1 << 2
} LogviewLevelFlags;

void logview_debug_init ();
void logview_debug_destroy ();
void lv_debug (LogviewLevelFlags level,
	       const gchar *file,
	       gint line,
	       const gchar *function);
void lv_debug_message (LogviewLevelFlags level,
		       const gchar *file,
		       gint line,
		       const gchar *function,
		       const gchar *format, ...);

#ifdef ON_SUN_OS
#define FUNC __func__
#else
#define FUNC G_STRFUNC
#endif

/**
 * LV_MARK:
 * 
 * Dump out information in the %LOGVIEW_ERR level.
 **/
#define LV_MARK \
	lv_debug (LOGVIEW_ERR, __FILE__, __LINE__, FUNC)

/**
 * LV_INFO:
 * @FORMAT: a formatted string.
 * @Varargs: 
 * 
 * Dump out information in the %LOGVIEW_VERBOSE and %LOGVIEW_INFO level.
 **/
#define LV_INFO(FORMAT, ...)  \
	lv_debug_message (LOGVIEW_INFO, __FILE__, __LINE__, FUNC, "[II] " FORMAT, __VA_ARGS__)

/**
 * LV_INFO_WW:
 * @FORMAT: a formatted string.
 * @Varargs: 
 * 
 * Like LV_INFO(), dump out warnning information.
 **/
#define LV_INFO_WW(FORMAT, ...)  \
	lv_debug_message (LOGVIEW_INFO, __FILE__, __LINE__, FUNC, "[WW] " FORMAT, __VA_ARGS__)

/**
 * LV_INFO_EE:
 * @FORMAT: a formatted string.
 * @Varargs: 
 * 
 * Like LV_INFO(), dump out error information.
 **/
#define LV_INFO_EE(FORMAT, ...)  \
	lv_debug_message (LOGVIEW_INFO, __FILE__, __LINE__, FUNC, "[EE] " FORMAT, __VA_ARGS__)

/**
 * LV_ERR:
 * @FORMAT: a formatted string.
 * @Varargs: 
 * 
 * Dump out information in the %LOGVIEW_ERR level.
 **/
#define LV_ERR(FORMAT, ...)  \
	lv_debug_message (LOGVIEW_ERR, __FILE__, __LINE__, FUNC, FORMAT, __VA_ARGS__)

#endif
