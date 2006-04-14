/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * envelope.c: envelope generator for gstreamer
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "envelope.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

enum {
  // class properties
  PROP_VALUE=1,
};

static GObjectClass *parent_class = NULL;

//-- constructor methods

/**
 * gst_envelope_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstEnvelope *gst_envelope_new(void) {
  GstEnvelope *self;

  self=GST_ENVELOPE(g_object_new(GST_TYPE_ENVELOPE,NULL));
  return(self);
}

//-- private methods

//-- public methods

static void
gst_envelope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEnvelope *env = GST_ENVELOPE (object);

  if (env->dispose_has_run) return;

  switch (prop_id) {
    case PROP_VALUE:
      env->value = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_envelope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEnvelope *env = GST_ENVELOPE (object);

  if (env->dispose_has_run) return;

  switch (prop_id) {
    case PROP_VALUE:
      // @todo: gst_object_sync_values (G_OBJECT (env), env->running_time);
      g_value_set_double (value, env->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_envelope_dispose (GObject *object)
{
  GstEnvelope *env = GST_ENVELOPE (object);

  if (env->dispose_has_run) return;
  env->dispose_has_run = TRUE;
  
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void
gst_envelope_init (GstEnvelope * env, GstEnvelopeClass * g_class)
{
  env->value=0.0;
}

static void
gst_envelope_class_init (GstEnvelopeClass * klass)
{
  GObjectClass *gobject_class;

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "envelope", GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");
  
  parent_class = (GObjectClass *) g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_envelope_set_property;
  gobject_class->get_property = gst_envelope_get_property;
  gobject_class->dispose      = gst_envelope_dispose;
  
  // register own properties

  g_object_class_install_property(gobject_class, PROP_VALUE,
  	g_param_spec_double("value", "Value",
          "Current envelope value",
          0.0, 1.0, 0.0, G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));

}

GType gst_envelope_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo element_type_info = {
      sizeof (GstEnvelopeClass),
      NULL,               /* base_init */
      NULL,		  /* base_finalize */
      (GClassInitFunc)gst_envelope_class_init,
      NULL,		  /* class_finalize */
      NULL,               /* class_data */
      sizeof (GstEnvelope),
      0,                  /* n_preallocs */
      (GInstanceInitFunc) gst_envelope_init
    };
    type = g_type_register_static(G_TYPE_OBJECT, "GstEnvelope", &element_type_info, (GTypeFlags) 0);
  }
  return type;
}
