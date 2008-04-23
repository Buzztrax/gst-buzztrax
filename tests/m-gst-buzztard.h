/* $Id$ */

#include <signal.h>
#include "check.h"
#include "glib.h"
#include "gst/gst.h"

#include "libgstbuzztard/note2frequency.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(gst_buzztard_debug);

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
