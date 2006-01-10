/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * simsyn.c: simple audio synthesizer for gstreamer
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

#ifndef __GST_AUDIO_TEST_SRC_H__
#define __GST_SIM_SYN_H__


#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS


#define GST_TYPE_SIM_SYN \
  (gst_sim_syn_get_type())
#define GST_SIM_SYN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SIM_SYN,GstSimSyn))
#define GST_SIM_SYN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SIM_SYN,GstSimSynClass))
#define GST_IS_SIM_SYN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SIM_SYN))
#define GST_IS_SIM_SYN_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SIM_SYN))

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

  /* parameters */
  GstSimSynWaves wave;
  gdouble volume;
  gdouble freq;
    
  /* audio parameters */
  gint samplerate;
  gint samples_per_buffer;
  
  /* < private > */
  gboolean tags_pushed;			/* send tags just once ? */
  GstClockTimeDiff timestamp_offset;    /* base offset */
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_seek_stop;
  gboolean eos_reached;
  gint generate_samples_per_buffer;	/* used to generate a partial buffer */
  
  /* waveform specific context data */
  gdouble accumulator;			/* phase angle */
  GstPinkNoise pink;
  gint16 wave_table[1024];
};

struct _GstSimSynClass {
  GstBaseSrcClass parent_class;
};

GType gst_sim_syn_get_type(void);
gboolean gst_sim_syn_factory_init (GstElementFactory *factory);

G_END_DECLS

#endif /* __GST_SIM_SYN_H__ */