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

#ifndef __GSTBT_TONE_CONVERSION_H__
#define __GSTBT_TONE_CONVERSION_H__

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TONE_CONVERSION_NOTE_NO 0
//#define GSTBT_TONE_CONVERSION_NOTE_MIN 1
//#define GSTBT_TONE_CONVERSION_NOTE_MAX 1+(9*16)+11
#define GSTBT_TONE_CONVERSION_NOTE_OFF 255

#define GSTBT_TYPE_TONE_CONVERSION_TUNING     (gstbt_tone_conversion_tuning_get_type())

/**
 * GstBtToneConversionTuning:
 * @GSTBT_TONE_CONVERSION_CROMATIC: 12 tones with equal distance (equal temperament)
 *
 * Supported tuning types.
 * see http://en.wikipedia.org/wiki/Musical_tuning
 */
typedef enum {
  /* 12 tones with equal distance (equal temperament) */
  GSTBT_TONE_CONVERSION_CROMATIC=0,
  /* @todo: add more */
} GstBtToneConversionTuning;

#define GSTBT_TYPE_TONE_CONVERSION            (gstbt_tone_conversion_get_type ())
#define GSTBT_TONE_CONVERSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversion))
#define GSTBT_TONE_CONVERSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversionClass))
#define GSTBT_IS_TONE_CONVERSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_TONE_CONVERSION))
#define GSTBT_IS_TONE_CONVERSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSTBT_TYPE_TONE_CONVERSION))
#define GSTBT_TONE_CONVERSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversionClass))

typedef struct _GstBtToneConversion GstBtToneConversion;
typedef struct _GstBtToneConversionClass GstBtToneConversionClass;

/**
 * GstBtToneConversion:
 *
 * Opaque object instance.
 */
struct _GstBtToneConversion {
  GObject parent;
  
  /*< private >*/
  GstBtToneConversionTuning tuning;
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  /* translate callback */
  gdouble (*translate)(GstBtToneConversion*, guint, guint);
};

struct _GstBtToneConversionClass {
  GObjectClass parent;
  
};

GType gstbt_tone_conversion_get_type(void) G_GNUC_CONST;
GType gstbt_tone_conversion_tuning_get_type(void) G_GNUC_CONST;

GstBtToneConversion *gstbt_tone_conversion_new(GstBtToneConversionTuning tuning);
gdouble gstbt_tone_conversion_translate_from_string(GstBtToneConversion *self,gchar *note);
gdouble gstbt_tone_conversion_translate_from_number(GstBtToneConversion *self,guint note);

guint gstbt_tone_conversion_note_string_2_number(const gchar *note);
const gchar *gstbt_tone_conversion_note_number_2_string(guint note);

G_END_DECLS

#endif /* __GSTBT_TONE_CONVERSION_H__ */
