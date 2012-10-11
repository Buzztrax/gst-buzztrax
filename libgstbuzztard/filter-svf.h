/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * filter-svf.h: state variable filter
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

#ifndef __GSTBT_FILTER_SVF_H__
#define __GSTBT_FILTER_SVF_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_FILTER_SVF_TYPE (gstbt_filter_svf_type_get_type())

/**
 * GstBtFilterSVFType:
 * @GSTBT_FILTER_SVF_NONE: no filtering
 * @GSTBT_FILTER_SVF_LOWPASS: low pass
 * @GSTBT_FILTER_SVF_HIPASS: high pass
 * @GSTBT_FILTER_SVF_BANDPASS: band pass
 * @GSTBT_FILTER_SVF_BANDSTOP: band stop (notch)
 *
 * Filter types.
 */
typedef enum
{
  GSTBT_FILTER_SVF_NONE,
  GSTBT_FILTER_SVF_LOWPASS,
  GSTBT_FILTER_SVF_HIPASS,
  GSTBT_FILTER_SVF_BANDPASS,
  GSTBT_FILTER_SVF_BANDSTOP
} GstBtFilterSVFType;

GType gstbt_filter_svf_type_get_type(void);


#define GSTBT_TYPE_FILTER_SVF            (gstbt_filter_svf_get_type())
#define GSTBT_FILTER_SVF(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_FILTER_SVF,GstBtFilterSVF))
#define GSTBT_IS_FILTER_SVF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_FILTER_SVF))
#define GSTBT_FILTER_SVF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_FILTER_SVF,GstBtFilterSVFClass))
#define GSTBT_IS_FILTER_SVF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_FILTER_SVF))
#define GSTBT_FILTER_SVF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_FILTER_SVF,GstBtFilterSVFClass))

typedef struct _GstBtFilterSVF GstBtFilterSVF;
typedef struct _GstBtFilterSVFClass GstBtFilterSVFClass;

/**
 * GstBtFilterSVF:
 * @value: current envelope value
 *
 * Class instance data.
 */
struct _GstBtFilterSVF {
  GObject parent;

  /* < public > */
  /* parameters */
  GstBtFilterSVFType type;
  gdouble cutoff, resonance;

  /* filter state */
  gdouble flt_low, flt_mid, flt_high;
  gdouble flt_res;

  /* < private > */
  void (*process) (GstBtFilterSVF *, guint, gint16 *);
  
  gboolean dispose_has_run;		/* validate if dispose has run */
};

struct _GstBtFilterSVFClass {
  GObjectClass parent_class;
};

GType gstbt_filter_svf_get_type(void);

GstBtFilterSVF *gstbt_filter_svf_new(void);

G_END_DECLS
#endif /* __GSTBT_FILTER_SVF_H__ */
