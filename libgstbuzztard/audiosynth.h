/* GStreamer
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

#ifndef __GSTBT_AUDIOSYNTH_H__
#define __GSTBT_AUDIOSYNTH_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/controller/gstcontroller.h>

G_BEGIN_DECLS
#define GSTBT_TYPE_AUDIOSYNTH			        (gstbt_audiosynth_get_type())
#define GSTBT_AUDIOSYNTH(obj)			        (G_TYPE_CHECK_INSTANCE_CAST((obj), GSTBT_TYPE_AUDIOSYNTH,GstBtAudioSynth))
#define GSTBT_IS_AUDIOSYNTH(obj)		      (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSTBT_TYPE_AUDIOSYNTH))
#define GSTBT_AUDIOSYNTH_CLASS(klass)	    (G_TYPE_CHECK_CLASS_CAST((klass),GSTBT_TYPE_AUDIOSYNTH,GstBtAudioSynthClass))
#define GSTBT_IS_AUDIOSYNTH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), GSTBT_TYPE_AUDIOSYNTH))
#define GSTBT_AUDIOSYNTH_GET_CLASS(ob)	  (G_TYPE_INSTANCE_GET_CLASS((obj), GSTBT_TYPE_AUDIOSYNTH,GstBtAudioSynthClass))
typedef struct _GstBtAudioSynth GstBtAudioSynth;
typedef struct _GstBtAudioSynthClass GstBtAudioSynthClass;

typedef void (*GstBtAudioSynthProcessFunc) (GstBtAudioSynth *, gint16 *);

/**
 * GstBtAudioSynth:
 *
 * Class instance data.
 */

struct _GstBtAudioSynth
{
  GstBaseSrc parent;

  /* < private > */
  /* parameters */
  gdouble samples_per_buffer;

  gboolean dispose_has_run;     /* validate if dispose has run */

  gint samplerate;
  GstClockTime running_time;    /* total running time */
  gint64 n_samples;             /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_eos;
  gboolean eos_reached;
  guint generate_samples_per_buffer;    /* generate a partial buffer */
  gboolean reverse;             /* play backwards */

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  GstClockTime ticktime;
  gdouble ticktime_err, ticktime_err_accum;
};

struct _GstBtAudioSynthClass
{
  GstBaseSrcClass parent_class;

  /* virtual function */
  GstBtAudioSynthProcessFunc process;
};

GType gstbt_audiosynth_get_type (void);

G_END_DECLS
#endif /* __GSTBT_AUDIOSYNTH_H__ */
