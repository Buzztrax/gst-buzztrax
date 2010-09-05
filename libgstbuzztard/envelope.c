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
/**
 * SECTION:gstbtenvelope
 * @short_description: envelope helper object
 *
 * Object with one controllable public gdouble variable /property
 * (#GstBtEnvelope:value).
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
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum {
  // class properties
  PROP_VALUE=1,
};

//-- the class

G_DEFINE_TYPE (GstBtEnvelope, gstbt_envelope, G_TYPE_OBJECT);

//-- constructor methods

/**
 * gstbt_envelope_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelope *gstbt_envelope_new(void) {
  GstBtEnvelope *self;

  self=GSTBT_ENVELOPE(g_object_new(GSTBT_TYPE_ENVELOPE,NULL));
  return(self);
}

//-- private methods

//-- public methods

static void
gstbt_envelope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *env = GSTBT_ENVELOPE (object);

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
gstbt_envelope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *env = GSTBT_ENVELOPE (object);

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
gstbt_envelope_dispose (GObject *object)
{
  GstBtEnvelope *env = GSTBT_ENVELOPE (object);

  if (env->dispose_has_run) return;
  env->dispose_has_run = TRUE;
  
  G_OBJECT_CLASS(gstbt_envelope_parent_class)->dispose(object);
}

static void
gstbt_envelope_init (GstBtEnvelope * env)
{
  env->value=0.0;
}

static void
gstbt_envelope_class_init (GstBtEnvelopeClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "envelope", GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  gobject_class->set_property = gstbt_envelope_set_property;
  gobject_class->get_property = gstbt_envelope_get_property;
  gobject_class->dispose      = gstbt_envelope_dispose;
  
  // register own properties

  g_object_class_install_property(gobject_class, PROP_VALUE,
  	g_param_spec_double("value", "Value",
          "Current envelope value",
          0.0, 1.0, 0.0,
          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|GST_PARAM_CONTROLLABLE));

}

