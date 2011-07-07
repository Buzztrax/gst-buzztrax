/* GStreamer
 * Copyright (C) 2007 Stefan Kost <ensonic@users.sf.net>
 *
 * musicenums.h: enum types for gstreamer elements
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

#ifndef __GSTBT_MUSICENUMS_H__
#define __GSTBT_MUSICENUMS_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GstBtTriggerSwitch:
 * @GSTBT_TRIGGER_SWITCH_OFF: turn switch off
 * @GSTBT_TRIGGER_SWITCH_ON: turn switch on
 * @GSTBT_TRIGGER_SWITCH_EMPTY: do not change switch
 *
 * Switch event commands.
 */
typedef enum {
  GSTBT_TRIGGER_SWITCH_OFF=0,
  GSTBT_TRIGGER_SWITCH_ON=1,
  GSTBT_TRIGGER_SWITCH_EMPTY=255,
} GstBtTriggerSwitch;

#define GSTBT_TYPE_TRIGGER_SWITCH   (gstbt_trigger_switch_get_type())

extern GType gstbt_trigger_switch_get_type(void);

G_END_DECLS

#endif /* __GSTBT_MUSICENUMS_H__ */
