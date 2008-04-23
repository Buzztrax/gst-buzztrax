/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * tempo.h: helper interface header for tempo synced gstreamer elements
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

#ifndef __GST_TEMPO_H__
#define __GST_TEMPO_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_TEMPO               (gst_tempo_get_type())
#define GST_TEMPO(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_TEMPO, GstTempo))
#define GST_IS_TEMPO(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_TEMPO))
#define GST_TEMPO_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GST_TYPE_TEMPO, GstTempoInterface))


typedef struct _GstTempo GstTempo; /* dummy object */
typedef struct _GstTempoInterface GstTempoInterface;

struct _GstTempoInterface
{
  GTypeInterface parent;

  void (*change_tempo) (GstTempo *self, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick);
};

GType gst_tempo_get_type(void);

void gst_tempo_change_tempo (GstTempo *self, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick);

G_END_DECLS

#endif /* __GST_TEMPO_H__ */
