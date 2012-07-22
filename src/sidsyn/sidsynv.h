/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsync.h: c64 sid synthesizer voice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
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

#ifndef __GSTBT_SID_SYNV_V_H__
#define __GSTBT_SID_SYNV_V_H__

#include <gst/gst.h>
#include <libgstbuzztard/musicenums.h>

G_BEGIN_DECLS
#define GSTBT_TYPE_SID_SYNV            (gstbt_sid_synv_get_type())
#define GSTBT_SID_SYNV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_SID_SYNV,GstBtSidSynV))
#define GSTBT_IS_SID_SYNV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_SID_SYNV))
#define GSTBT_SID_SYNV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_SID_SYNV,GstBtSidSynVClass))
#define GSTBT_IS_SID_SYNV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_SID_SYNV))
#define GSTBT_SID_SYNV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_SID_SYNV,GstBtSidSynVClass))

/**
 * GstBtSidSynWave:
 * @GSTBT_SID_SYN_WAVE_TRIANGLE: triangle wave
 * @GSTBT_SID_SYN_WAVE_SAW: saw wave
 * @GSTBT_SID_SYN_WAVE_SQUARE: square wave
 * @GSTBT_SID_SYN_WAVE_NOISE: noise
 *
 * Oscillator wave forms.
 */
typedef enum
{
  GSTBT_SID_SYN_WAVE_TRIANGLE = (1<<4),
  GSTBT_SID_SYN_WAVE_SAW = (1<<5),
  GSTBT_SID_SYN_WAVE_SQUARE = (1<<6),
  GSTBT_SID_SYN_WAVE_NOISE = (1<<7)
} GstBtSimSynWave;

typedef struct _GstBtSidSynV GstBtSidSynV;
typedef struct _GstBtSidSynVClass GstBtSidSynVClass;

/**
 * GstBtSidSynV:
 *
 * Class instance data.
 */
struct _GstBtSidSynV
{
  GstObject parent;

  /* < private > */
  /* parameters */
  GstBtNote note;
  gboolean note_set, gate;
  gboolean sync, ringmod, filter;
  GstBtSimSynWave wave;
  guint pulse_width;
  guint attack, decay, sustain, release;
};

struct _GstBtSidSynVClass
{
  GstObjectClass parent_class;
};

GType gstbt_sid_synv_get_type (void);

G_END_DECLS
#endif /* __GSTBT_SID_SYN_H_V__ */
