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

#ifndef __GSTBT_SIM_SYN_H__
#define __GSTBT_SIM_SYN_H__


#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/controller/gstcontroller.h>
#include <libgstbuzztard/toneconversion.h>
#include <libgstbuzztard/envelope.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_SIM_SYN            (gstbt_sim_syn_get_type())
#define GSTBT_SIM_SYN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_SIM_SYN,GstBtSimSyn))
#define GSTBT_IS_SIM_SYN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_SIM_SYN))
#define GSTBT_SIM_SYN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_SIM_SYN,GstBtSimSynClass))
#define GSTBT_IS_SIM_SYN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_SIM_SYN))
#define GSTBT_SIM_SYN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_SIM_SYN,GstBtSimSynClass))

/**
 * GstBtSimSynWave:
 * @GSTBT_SIM_SYN_WAVE_SINE: sine wave
 * @GSTBT_SIM_SYN_WAVE_SQUARE: square wave
 * @GSTBT_SIM_SYN_WAVE_SAW: saw wave
 * @GSTBT_SIM_SYN_WAVE_TRIANGLE: triangle wave
 * @GSTBT_SIM_SYN_WAVE_SILENCE: silence
 * @GSTBT_SIM_SYN_WAVE_WHITE_NOISE: white noise
 * @GSTBT_SIM_SYN_WAVE_PINK_NOISE: pink noise
 * @GSTBT_SIM_SYN_WAVE_SINE_TAB: sine wave (precalculated)
 *
 * Oscillator wave forms.
 */
typedef enum {
  GSTBT_SIM_SYN_WAVE_SINE,
  GSTBT_SIM_SYN_WAVE_SQUARE,
  GSTBT_SIM_SYN_WAVE_SAW,
  GSTBT_SIM_SYN_WAVE_TRIANGLE,
  GSTBT_SIM_SYN_WAVE_SILENCE,
  GSTBT_SIM_SYN_WAVE_WHITE_NOISE,
  GSTBT_SIM_SYN_WAVE_PINK_NOISE,
  GSTBT_SIM_SYN_WAVE_SINE_TAB
} GstBtSimSynWave;

/**
 * GstBtSimSynFilter:
 * @GSTBT_SIM_SYN_FILTER_NONE: no filtering
 * @GSTBT_SIM_SYN_FILTER_LOWPASS: low pass
 * @GSTBT_SIM_SYN_FILTER_HIPASS: high pass
 * @GSTBT_SIM_SYN_FILTER_BANDPASS: band pass
 * @GSTBT_SIM_SYN_FILTER_BANDSTOP: band stop (notch)
 *
 * Filter types.
 */
typedef enum {
  GSTBT_SIM_SYN_FILTER_NONE,
  GSTBT_SIM_SYN_FILTER_LOWPASS,
  GSTBT_SIM_SYN_FILTER_HIPASS,
  GSTBT_SIM_SYN_FILTER_BANDPASS,
  GSTBT_SIM_SYN_FILTER_BANDSTOP
} GstBtSimSynFilter;

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

#define WAVE_TABLE_SIZE 1024

typedef struct _GstBtSimSyn GstBtSimSyn;
typedef struct _GstBtSimSynClass GstBtSimSynClass;

/**
 * GstBtSimSyn:
 * @parent: parent instance
 *
 * Class instance data.
 */
struct _GstBtSimSyn {
  GstBaseSrc parent;

  /* < private > */
  /* parameters */
  gdouble samples_per_buffer;
  GstBtSimSynWave wave;
  gchar *note;
  gdouble volume;
  gdouble decay;
  GstBtSimSynFilter filter;
  gdouble cutoff;
  gdouble resonance;

  gboolean dispose_has_run;		/* validate if dispose has run */

  void (*process)(GstBtSimSyn*, gint16 *);
  void (*apply_filter)(GstBtSimSyn*, gint16 *);

  gint samplerate;
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_eos;
  gboolean eos_reached;
  guint generate_samples_per_buffer;	/* generate a partial buffer */
  gboolean reverse;                  /* play backwards */

  GstBtToneConversion *n2f;
  gdouble freq;
  guint64 note_count;
  GstBtEnvelope *volenv;                  /* volume-envelope */
  GstController *volenv_controller;     /* volume-envelope controller */

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  GstClockTime ticktime;

  /* waveform specific context data */
  gdouble accumulator;			/* phase angle */
  GstPinkNoise pink;
  gint16 wave_table[ WAVE_TABLE_SIZE];

  /* filter specific data */
  gdouble flt_low, flt_mid, flt_high;
  gdouble flt_res;
};

struct _GstBtSimSynClass {
  GstBaseSrcClass parent_class;
};

GType gstbt_sim_syn_get_type(void);

G_END_DECLS

#endif /* __GSTBT_SIM_SYN_H__ */
