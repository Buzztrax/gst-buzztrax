/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#include <signal.h>
#include "check.h"
#include "glib.h"
#include "gst/gst.h"

#include "libgstbuzztard/toneconversion.h"

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
