/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * simsyn.h: simple audio synthesizer for gstreamer
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

#ifndef __GST_SIM_SYN_H__
#define __GST_SIM_SYN_H__


#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <note2frequency/note2frequency.h>
#include <envelope/envelope.h>

G_BEGIN_DECLS


#define GST_TYPE_SIM_SYN            (gst_sim_syn_get_type())
#define GST_SIM_SYN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SIM_SYN,GstSimSyn))
#define GST_IS_SIM_SYN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SIM_SYN))
#define GST_SIM_SYN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_SIM_SYN,GstSimSynClass))
#define GST_IS_SIM_SYN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_SIM_SYN))
#define GST_SIM_SYN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_SIM_SYN,GstSimSynClass))

typedef enum {
  GST_SIM_SYN_WAVE_SINE,
  GST_SIM_SYN_WAVE_SQUARE,
  GST_SIM_SYN_WAVE_SAW,
  GST_SIM_SYN_WAVE_TRIANGLE,
  GST_SIM_SYN_WAVE_SILENCE,
  GST_SIM_SYN_WAVE_WHITE_NOISE,
  GST_SIM_SYN_WAVE_PINK_NOISE,
  GST_SIM_SYN_WAVE_SINE_TAB
} GstSimSynWaves; 

typedef enum {
  GST_SIM_SYN_FILTER_NONE,
  GST_SIM_SYN_FILTER_LOWPASS,
  GST_SIM_SYN_FILTER_HIPASS,
  GST_SIM_SYN_FILTER_BANDPASS,
  GST_SIM_SYN_FILTER_BANDSTOP
} GstSimSynFilter;

#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (16)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

typedef struct {
  glong      rows[PINK_MAX_RANDOM_ROWS];
  glong      running_sum;   /* Used to optimize summing of generators. */
  gint       index;         /* Incremented each sample. */
  gint       index_mask;    /* Index wrapped by ANDing with this mask. */
  gfloat     scalar;        /* Used to scale within range of -1.0 to +1.0 */
} GstPinkNoise;

typedef struct _GstSimSyn GstSimSyn;
typedef struct _GstSimSynClass GstSimSynClass;

struct _GstSimSyn {
  GstBaseSrc parent;

  void (*process)(GstSimSyn*, gint16 *);
  void (*apply_filter)(GstSimSyn*, gint16 *);

  /* parameters */
  gdouble samples_per_buffer;
  GstSimSynWaves wave;
  gchar *note;
  gdouble volume;
  gdouble decay;
  GstSimSynFilter filter;
  gdouble cutoff;
  gdouble resonance;
  
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */

  gint samplerate;
  GstClockTimeDiff timestamp_offset;    /* base offset */
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_seek_stop;
  gboolean eos_reached;
  gint generate_samples_per_buffer;	/* generate a partial buffer */

  GstNote2Frequency *n2f;
  gdouble freq;
  guint64 note_count;
  GstEnvelope *volenv;                  /* volume-envelope */
  GstController *volenv_controller;     /* volume-envelope controller */

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  GstClockTime ticktime;

  /* waveform specific context data */
  gdouble accumulator;			/* phase angle */
  GstPinkNoise pink;
  gint16 wave_table[1024];
  
  /* filter specific data */
  gdouble flt_low, flt_mid, flt_high;
  gdouble flt_res;
};

struct _GstSimSynClass {
  GstBaseSrcClass parent_class;
};

GType gst_sim_syn_get_type(void);

G_END_DECLS

#endif /* __GST_SIM_SYN_H__ */
