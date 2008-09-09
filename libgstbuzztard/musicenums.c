/* GStreamer
 * Copyright (C) 2007 Stefan Kost <ensonic@users.sf.net>
 *
 * musicenums.c: enum types for gstreamer elements
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
/**
 * SECTION:musicenums
 * @short_description: various enum types
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicenums.h"

GType gstGSTBT_TRIGGER_SWITCH_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static const GEnumValue values[] = {
      { GSTGSTBT_TRIGGER_SWITCH_OFF,  "OFF","0" },
      { GSTGSTBT_TRIGGER_SWITCH_ON,   "ON","1" },
      { GSTGSTBT_TRIGGER_SWITCH_EMPTY,"EMPTY","" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtTriggerSwitch", values);
  }
  return type;
}

