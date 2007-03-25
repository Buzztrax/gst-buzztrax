/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * tempo.c: helper interface for tempo synced gstreamer elements
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
 * SECTION:gsttempo
 * @short_description: helper interface for tempo synced gstreamer elements
 *
 * This interface offers three #GObject properties to specify a rythmic tempo of
 * the GStreamer pipeline. The tempo can be changed from the application side by
 * calling gst_tempo_change_tempo().
 *
 * #GstElements that implement this interface should use the given tempo, to
 * adjust their gst_object_sync_values() cycle.
 *
 * The difference between the tempo iface and the tempo-tag metadata is that the
 * metadata describes the overall tempo, but the iface allows to change the
 * tempo during playback.
 */

#include "tempo.h"

/**
 * gst_tempo_change_tempo:
 * @self: a #GObject that implements #GstTempo
 * @beats_per_minute: the number of beats in a minute
 * @ticks_per_beat: the number of ticks of one beat (measure)
 * @subticks_per_tick: the number of subticks within one tick
 *
 * Set all tempo properties at once. Pass -1 to leave a value unchanged.
 */
void
gst_tempo_change_tempo (GstTempo *self, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick)
{
  g_return_if_fail (GST_IS_TEMPO (self));

  GST_TEMPO_GET_INTERFACE (self)->change_tempo (self, beats_per_minute, ticks_per_beat, subticks_per_tick);
}

static void
gst_tempo_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */
    g_object_interface_install_property (g_class,
      g_param_spec_ulong ("beats-per-minute",
      "beat-per-minute tempo property",
      "the number of beats per minute the top level pipeline uses",
      1,
      G_MAXULONG,
      120,
      G_PARAM_READWRITE));
    g_object_interface_install_property (g_class,
      g_param_spec_ulong ("ticks-per-beat",
      "ticks-per-beat tempo property",
      "the number of ticks (events) per beat the top level pipeline uses",
      1,
      G_MAXULONG,
      4,
      G_PARAM_READWRITE));
    g_object_interface_install_property (g_class,
      g_param_spec_ulong ("subticks-per-tick",
      "subticks-per-tick tempo property",
      "the number of subticks (for smoothing) per tick the top level pipeline uses",
      1,
      G_MAXULONG,
      1,
      G_PARAM_READWRITE));
    initialized = TRUE;
  }
}

GType
gst_tempo_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstTempoInterface),
      (GBaseInitFunc) gst_tempo_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstTempo",&info,0);
  }
  return type;
}
