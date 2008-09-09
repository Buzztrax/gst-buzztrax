/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at users.sf.net>
 *
 * This library is free software; you can redigst_bml_instantiatestribute it and/or
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

#ifndef __GST_BML_PLUGIN_H__
#define __GST_BML_PLUGIN_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <math.h>
//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
//-- gstreamer
#include <gst/gst.h>
#include <gst/gstchildproxy.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/gstcontroller.h>
#include <gst/audio/audio.h>
//-- gstbuzztard
#include <libgstbuzztard/childbin.h>
#include <libgstbuzztard/help.h>
#include <libgstbuzztard/musicenums.h>
#include <libgstbuzztard/toneconversion.h>
#ifndef HAVE_GST_GSTPRESET_H
#include <libgstbuzztard/preset.h>
#endif
#include <libgstbuzztard/propertymeta.h>
#include <libgstbuzztard/tempo.h>
//-- liboil
#include <liboil/liboil.h>

//-- libbml
#include <bml.h>

#ifdef BML_WRAPPED
#define bml(a) bmlw_ ## a
#define BML(a) BMLW_ ## a
#else
#ifdef BML_NATIVE
#define bml(a) bmln_ ## a
#define BML(a) BMLN_ ## a
#else
#define bml(a) a
#define BML(a) a
#endif
#endif

#include "gstbml.h"
#include "gstbmlsrc.h"
#include "gstbmltransform.h"
#include "gstbmlv.h"
#include "common.h"          /* gstbml sdk utility functions */
#include "utils.h"           /* gstbml sdk utility functions */

#ifndef BML_VERSION
#define BML_VERSION "1.0"
#endif

#endif /* __GST_BML_PLUGIN_H_ */
