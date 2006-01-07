/* $Id: m-gst-buzztard.h,v 1.1 2006-01-07 16:50:05 ensonic Exp $ */

#include <signal.h>
#include "check.h"
#include "glib.h"

#include "note2frequency/note2frequency.h"

//-- globals

//GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);

//-- prototypes

extern void gst_buzztard_setup(void);
extern void gst_buzztard_teardown(void);

//-- testing helper methods

#define g_object_checked_unref(obj) \
{\
  gpointer __objref;\
  g_assert(obj);\
  g_object_add_weak_pointer((gpointer)obj,&__objref);\
  g_object_unref((gpointer)obj);\
  fail_unless(__objref == NULL, NULL);\
}
