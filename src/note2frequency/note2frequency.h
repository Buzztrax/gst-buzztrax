/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * note2frequency.c: helper class header for unit conversion
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

#ifndef __GST_NOTE_2_FREQUENCY_H__
#define __GST_NOTE_2_FREQUENCY_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GST_TYPE_NOTE_2_FREQUENCY_TUNING     (gst_note_2_frequency_tuning_get_type())

/*
 * see http://en.wikipedia.org/wiki/Musical_tuning
 */
typedef enum {
  /* 12 tones with equal distance (equal temperament) */
  GST_NOTE_2_FREQUENCY_CROMATIC=0,
  /* @todo: add more */
} GstNote2FrequencyTuning;

#define GST_TYPE_NOTE_2_FREQUENCY            (gst_note_2_frequency_get_type ())
#define GST_NOTE_2_FREQUENCY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_NOTE_2_FREQUENCY, GstNote2Frequency))
#define GST_NOTE_2_FREQUENCY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_NOTE_2_FREQUENCY, GstNote2FrequencyClass))
#define GST_IS_NOTE_2_FREQUENCY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_NOTE_2_FREQUENCY))
#define GST_IS_NOTE_2_FREQUENCY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_NOTE_2_FREQUENCY))
#define GST_NOTE_2_FREQUENCY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_NOTE_2_FREQUENCY, GstNote2FrequencyClass))

typedef struct _GstNote2Frequency GstNote2Frequency;
typedef struct _GstNote2FrequencyClass GstNote2FrequencyClass;

/**
 * GstNote2Frequency:
 */
struct _GstNote2Frequency {
  GObject parent;
  
  /*< private >*/
  GstNote2FrequencyTuning tuning;
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  /* translate callback */
  gdouble (*translate)(GstNote2Frequency*, gchar *);
};

struct _GstNote2FrequencyClass {
  GObjectClass parent;
  
};

GType gst_note_2_frequency_get_type(void) G_GNUC_CONST;
GType gst_note_2_frequency_tuning_get_type(void) G_GNUC_CONST;

GstNote2Frequency *gst_note_2_frequency_new(GstNote2FrequencyTuning tuning);
gdouble gst_note_2_frequency_translate(GstNote2Frequency *self,gchar *note);

G_END_DECLS

#endif /* __GST_NOTE_2_FREQUENCY_H__ */
