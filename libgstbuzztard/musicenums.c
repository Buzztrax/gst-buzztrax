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
 * SECTION:gstbtmusicenums
 * @title: GstBtMusicEnums
 * @short_description: various enum types
 *
 * Music/Audio related enumerations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicenums.h"

/**
 * GstBtTriggerSwitch:
 * @GSTBT_TRIGGER_SWITCH_OFF: turn switch off
 * @GSTBT_TRIGGER_SWITCH_ON: turn switch on
 * @GSTBT_TRIGGER_SWITCH_EMPTY: do not change switch
 *
 * Switch event commands.
 */
GType gstbt_trigger_switch_get_type(void) {
  static GType type = 0;

  if(G_UNLIKELY(!type)) {
    static const GEnumValue values[] = {
      { GSTBT_TRIGGER_SWITCH_OFF,  "OFF","0" },
      { GSTBT_TRIGGER_SWITCH_ON,   "ON","1" },
      { GSTBT_TRIGGER_SWITCH_EMPTY,"EMPTY","" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("GstBtTriggerSwitch", values);
  }
  return type;
}

