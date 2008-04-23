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

#ifndef __BT_MUSICENUMS_H__
#define __BT_MUSICENUMS_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  BT_TRIGGER_SWITCH_OFF=0,
  BT_TRIGGER_SWITCH_ON=1,
  BT_TRIGGER_SWITCH_EMPTY=255,
} BtTriggerSwitch;

#define BT_TYPE_TRIGGER_SWITCH   (bt_trigger_switch_get_type())

extern GType bt_trigger_switch_get_type(void);

G_END_DECLS

#endif /* __BT_MUSICENUMS_H__ */
