/* GStreamer
 * Copyright (C) 2007 Josh Green <josh@users.sf.net>
 *
 * Adapted from simsyn synthesizer plugin in gst-buzztard source.
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * gstfluidsynth.h: GStreamer wrapper for FluidSynth
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

#ifndef __GSTBT_FLUIDSYNTH_H__
#define __GSTBT_FLUIDSYNTH_H__

#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>
#include <libgstbuzztard/audiosynth.h>
#include <libgstbuzztard/envelope.h>
#include <libgstbuzztard/toneconversion.h>
#include <fluidsynth.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_FLUID_SYNTH            (gstbt_fluid_synth_get_type())
#define GSTBT_FLUID_SYNTH(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_FLUID_SYNTH,GstBtFluidsynth))
#define GSTBT_IS_FLUID_SYNTH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_FLUID_SYNTH))
#define GSTBT_FLUID_SYNTH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_FLUID_SYNTH,GstBtFluidsynthClass))
#define GSTBT_IS_FLUID_SYNTH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_FLUID_SYNTH))
#define GSTBT_FLUID_SYNTH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_FLUID_SYNTH,GstBtFluidsynthClass))

typedef struct _GstBtFluidsynth GstBtFluidsynth;
typedef struct _GstBtFluidsynthClass GstBtFluidsynthClass;

/**
 * GstBtFluidsynth:
 *
 * Class instance data.
 */
struct _GstBtFluidsynth {
  GstBtAudioSynth parent;

  /* < private > */
  /* parameters */
  gboolean dispose_has_run;         /* validate if dispose has run */

  GstBtNote note;
  gint key;
  gint velocity;
  glong cur_note_length,note_length;
  gint program;

  fluid_synth_t *fluid;			        /* the FluidSynth handle */
  fluid_settings_t *settings;       /* to free on close */
  fluid_midi_driver_t *midi;		    /* FluidSynth MIDI driver */
  fluid_midi_router_t *midi_router; /* FluidSynth MIDI router */

  gchar *instrument_patch_path;
  gint instrument_patch;

  int interp;				                /* interpolation type */

  gboolean reverb_enable;		        /* reverb enable */
  double reverb_room_size;		      /* reverb room size */
  double reverb_damp;			          /* reverb damping */
  double reverb_width;			        /* reverb width */
  double reverb_level;			        /* reverb level */
  gboolean reverb_update;

  gboolean chorus_enable;		        /* chorus enable */
  int chorus_count;			            /* chorus voice count */
  double chorus_level;			        /* chorus level */
  double chorus_freq;			          /* chorus freq */
  double chorus_depth;			        /* chorus depth */
  int chorus_waveform;			        /* chorus waveform */
  gboolean chorus_update;
};

struct _GstBtFluidsynthClass {
  GstBtAudioSynthClass parent_class;
};

GType gstbt_fluid_synth_get_type(void);

G_END_DECLS

#endif /* __GSTBT_FLUIDSYNTH_H__ */
