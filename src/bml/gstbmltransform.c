/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * gstbmltransform.c: BML transform plugin
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

// see also: gstidentity, gstvolume, gstlevel

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);

static GstStaticPadTemplate bml_pad_caps_mono_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (
    "audio/x-raw-float, "
    "width = (int) 32, "
    "rate = (int) [ 1, MAX ], "
    "channels = (int) 1, "
    "endianness = (int) BYTE_ORDER"
  )
);

static GstStaticPadTemplate bml_pad_caps_stereo_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (
    "audio/x-raw-float, "
    "width = (int) 32, "
    "rate = (int) [ 1, MAX ], "
    "channels = (int) 2, "
    "endianness = (int) BYTE_ORDER"
  )
);

static GstStaticPadTemplate bml_pad_caps_mono_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (
    "audio/x-raw-float, "
    "width = (int) 32, "
    "rate = (int) [ 1, MAX ], "
    "channels = (int) 1, "
    "endianness = (int) BYTE_ORDER"
  )
);
static GstStaticPadTemplate bml_pad_caps_stereo_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (
    "audio/x-raw-float, "
    "width = (int) 32, "
    "rate = (int) [ 1, MAX ], "
    "channels = (int) 2, "
    "endianness = (int) BYTE_ORDER"
  )
);

static GstBaseTransformClass *parent_class = NULL;

extern GHashTable *bml_descriptors_by_element_type;
extern GHashTable *bml_descriptors_by_voice_type;
extern GType voice_type;

//-- child bin interface implementations


//-- child proxy interface implementations

static GstObject *gst_bml_child_proxy_get_child_by_index (GstChildProxy *child_proxy, guint index) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(child_proxy);
  GstBML *bml=GST_BML(bml_transform);

  g_return_val_if_fail(index<bml->num_voices,NULL);

  return(g_object_ref(g_list_nth_data(bml->voices,index)));
}

static guint gst_bml_child_proxy_get_children_count (GstChildProxy *child_proxy) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(child_proxy);
  GstBML *bml=GST_BML(bml_transform);

  return(bml->num_voices);
}


static void gst_bml_child_proxy_interface_init(gpointer g_iface, gpointer iface_data) {
  GstChildProxyInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->get_child_by_index = gst_bml_child_proxy_get_child_by_index;
  iface->get_children_count = gst_bml_child_proxy_get_children_count;
}

//-- property meta interface implementations

static gchar *gst_bml_property_meta_describe_property(GstPropertyMeta *property_meta, glong index, GValue *event) {
  GstBML *bml=GST_BML(GST_BML_TRANSFORM(property_meta));

  return(bml(gstbml_property_meta_describe_property(bml->bm,index,event)));
}

static void gst_bml_property_meta_interface_init(gpointer g_iface, gpointer iface_data) {
  GstPropertyMetaInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->describe_property = gst_bml_property_meta_describe_property;
}

//-- tempo interface implementations

static void gst_bml_tempo_change_tempo(GstTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(tempo);
  GstBML *bml=GST_BML(bml_transform);

  bml(gstbml_tempo_change_tempo(G_OBJECT(bml_transform),bml,beats_per_minute,ticks_per_beat,subticks_per_tick));
}

static void gst_bml_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_bml_tempo_change_tempo;
}

//-- preset interface implementations

static gchar** gst_bml_preset_get_preset_names(GstPreset *preset) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBML *bml=GST_BML(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_get_preset_names(bml,bml_class));
}

static gboolean gst_bml_preset_load_preset(GstPreset *preset, const gchar *name) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBML *bml=GST_BML(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_load_preset(GST_OBJECT(preset),bml,bml_class,name));
}

static gboolean gst_bml_preset_save_preset(GstPreset *preset, const gchar *name) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBML *bml=GST_BML(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_save_preset(GST_OBJECT(preset),bml,bml_class,name));
}

static gboolean gst_bml_preset_rename_preset(GstPreset *preset, const gchar *old_name, const gchar *new_name) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_rename_preset(bml_class,old_name,new_name));
}

static gboolean gst_bml_preset_delete_preset(GstPreset *preset, const gchar *name) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_delete_preset(bml_class,name));
}

static gboolean gst_bml_set_meta (GstPreset *preset,const gchar *name, const gchar *tag, const gchar *value) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_set_meta(bml_class,name,tag,value));
}

static gboolean gst_bml_get_meta (GstPreset *preset,const gchar *name, const gchar *tag, gchar **value) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(preset);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_get_meta(bml_class,name,tag,value));
}

static void gst_bml_preset_interface_init(gpointer g_iface, gpointer iface_data) {
  GstPresetInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->get_preset_names = gst_bml_preset_get_preset_names;
  iface->load_preset = gst_bml_preset_load_preset;
  iface->save_preset = gst_bml_preset_save_preset;
  iface->rename_preset = gst_bml_preset_rename_preset;
  iface->delete_preset = gst_bml_preset_delete_preset;
  iface->set_meta = gst_bml_set_meta;
  iface->get_meta = gst_bml_get_meta;
}

//-- gstbmltransform class implementation

/* get notified of caps and reject unsupported ones */
static gboolean gst_bml_transform_set_caps(GstBaseTransform * base, GstCaps * incaps, GstCaps * outcaps) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(base);
  const gchar *mimetype;

  GST_INFO("set_caps: in %"GST_PTR_FORMAT"  out %"GST_PTR_FORMAT, incaps, outcaps);

  mimetype=gst_structure_get_name(gst_caps_get_structure(incaps,0));
  if(strcmp(mimetype,"audio/x-raw-float")) {
    GST_ELEMENT_ERROR (bml_transform, CORE, NEGOTIATION,
        ("Unsupported incoming caps: %" GST_PTR_FORMAT, incaps), (NULL));
    return FALSE;
  }

  return TRUE;
}

static GstFlowReturn gst_bml_transform_transform_ip_mono(GstBaseTransform *base, GstBuffer *outbuf) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(base);
  GstBML *bml=GST_BML(bml_transform);
  BMLData *data,*seg_data;
  gpointer bm=bml->bm;
  guint todo,seg_size,samples_per_buffer;
  gboolean has_data;
  guint mode=3; /*WM_READWRITE*/

  bml->running_time=gst_segment_to_stream_time(&base->segment,GST_FORMAT_TIME,GST_BUFFER_TIMESTAMP(outbuf));
  data=(BMLData *)GST_BUFFER_DATA (outbuf);
  samples_per_buffer=GST_BUFFER_SIZE(outbuf)/sizeof(BMLData);

  /* split up processing of the buffer into chunks so that params can
   * be updated when required (e.g. for the subticks-feature).
   */
  gstbml_sync_values(bml);
  bml(tick(bm));

  /* don't process data in passthrough-mode */
  if (gst_base_transform_is_passthrough (base))
    return GST_FLOW_OK;

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET(outbuf,GST_BUFFER_FLAG_GAP)) {
    mode=2; /* WM_WRITE */
  }
  else {
    // buzz generates relative loud output
    //for(i=0;i<samples_per_buffer;i++) data[i]*=32768.0f;
    gfloat fc=32768.0;
    oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer);
  }

  GST_DEBUG("  calling work(%d,%d)",samples_per_buffer,mode);
  todo = samples_per_buffer;
  seg_data = data;
  has_data=FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo>256) ? 256 : todo; 
    has_data |= bml(work(bm,seg_data,(int)seg_size,mode));
    seg_data = &seg_data[seg_size];
    todo -= seg_size;
  }
  if(!has_data) {
    GST_INFO_OBJECT(bml_transform,"silent buffer");
    GST_BUFFER_FLAG_SET(outbuf,GST_BUFFER_FLAG_GAP);
  }
  else {
    GST_BUFFER_FLAG_UNSET(outbuf,GST_BUFFER_FLAG_GAP);
    // buzz generates relative loud output
    //for(i=0;i<samples_per_buffer;i++) data[i]/=32768.0f;
    gfloat fc=1.0/32768.0;
    oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer);
  }

  return(GST_FLOW_OK);
}

static GstFlowReturn gst_bml_transform_transform_ip_stereo(GstBaseTransform *base, GstBuffer *outbuf) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(base);
  GstBML *bml=GST_BML(bml_transform);
  BMLData *data,*seg_data;
  gpointer bm=bml->bm;
  guint todo,seg_size,samples_per_buffer;
  gboolean has_data;
  guint mode=3; /*WM_READWRITE*/

  bml->running_time=gst_segment_to_stream_time(&base->segment,GST_FORMAT_TIME,GST_BUFFER_TIMESTAMP(outbuf));
  data=(BMLData *)GST_BUFFER_DATA (outbuf);
  samples_per_buffer=GST_BUFFER_SIZE(outbuf)/(sizeof(BMLData)*2);

  /* split up processing of the buffer into chunks so that params can
   * be updated when required (e.g. for the subticks-feature).
   */
  gstbml_sync_values(bml);
  bml(tick(bm));

  /* don't process data in passthrough-mode */
  if (gst_base_transform_is_passthrough (base))
    return GST_FLOW_OK;

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET(outbuf,GST_BUFFER_FLAG_GAP)) {
    mode=2; /* WM_WRITE */
  }
  else {
    //for(i=0;i<samples_per_buffer*2;i++) data[i]*=32768.0;
    gfloat fc=32768.0;
    oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer*2);
  }

  GST_DEBUG("  calling work_m2s(%d,%d)",samples_per_buffer,mode);
  todo = samples_per_buffer;
  seg_data = data;
  has_data=FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo>256) ? 256 : todo;
    // first seg_data can be NULL, its ignored
    has_data |= bml(work_m2s(bm,seg_data,seg_data,(int)seg_size,mode));
    seg_data = &seg_data[seg_size*2];
    todo -= seg_size;
  }
  if(!has_data) {
    GST_INFO_OBJECT(bml_transform,"silent buffer");
    GST_BUFFER_FLAG_SET(outbuf,GST_BUFFER_FLAG_GAP);
  }
  else {
    GST_BUFFER_FLAG_UNSET(outbuf,GST_BUFFER_FLAG_GAP);
    //for(i=0;i<samples_per_buffer*2;i++) data[i]/=32768.0;
    gfloat fc=1.0/32768.0;
    oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer*2);
  }

  return(GST_FLOW_OK);
}

static gboolean gst_bml_transform_get_unit_size (GstBaseTransform *base, GstCaps *caps, guint *size) {
  gint width, channels;
  GstStructure *structure;
  gboolean ret;

  g_assert (size);
  GST_INFO("get_unit_size: for %"GST_PTR_FORMAT, caps);

  /* this works for both float and int */
  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "width", &width);
  ret &= gst_structure_get_int (structure, "channels", &channels);

  *size = width * channels / 8;

  return ret;
}

static GstCaps *gst_bml_transform_transform_caps(GstBaseTransform * base, GstPadDirection direction, GstCaps * caps) {
  GstCaps *res;
  GstStructure *structure;

  /* transform caps gives one single caps so we can just replace
   * the channel property with our range. */
  res = gst_caps_copy (caps);
  structure = gst_caps_get_structure (res, 0);
  /* if we should produce this output, what can we accept */
  if (direction == GST_PAD_SRC) {
    GST_INFO_OBJECT (base, "allow 1 input channel");
    gst_structure_set (structure, 
      "channels", G_TYPE_INT, 1, 
      NULL);
  } else {
    GST_INFO_OBJECT (base, "allow 2 output channels");
    gst_structure_set (structure, 
      "channels", G_TYPE_INT, 2, 
      NULL);
  }

  return res;
}

static gboolean gst_bml_transform_stop(GstBaseTransform * base) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(base);
  GstBML *bml=GST_BML(bml_transform);
  gpointer bm=bml->bm;
  
  bml(stop(bm));
  return TRUE;
}

static GstFlowReturn gst_bml_transform_transform_mono_to_stereo(GstBaseTransform *base, GstBuffer *inbuf, GstBuffer *outbuf) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(base);
  GstBML *bml=GST_BML(bml_transform);
  BMLData *datai,*datao,*seg_datai,*seg_datao;
  gpointer bm=bml->bm;
  guint todo,seg_size,samples_per_buffer;
  gboolean has_data;
  guint mode=3; /*WM_READWRITE*/

  bml->running_time=gst_segment_to_stream_time(&base->segment,GST_FORMAT_TIME,GST_BUFFER_TIMESTAMP(inbuf));
  datai=(BMLData *)GST_BUFFER_DATA (inbuf);
  datao=(BMLData *)GST_BUFFER_DATA (outbuf);
  samples_per_buffer=GST_BUFFER_SIZE(inbuf)/sizeof(BMLData);
  
  // some buzzmachines expect a cleared buffer
  //for(i=0;i<samples_per_buffer;i++) datao[i]=0.0f;
  memset(datao,0,samples_per_buffer*sizeof(BMLData));
  
  GST_DEBUG("input : %p,%d  output: %p,%d",
    datai,GST_BUFFER_SIZE(inbuf),
    datao,GST_BUFFER_SIZE(outbuf));

  /* split up processing of the buffer into chunks so that params can
   * be updated when required (e.g. for the subticks-feature).
   */
  gstbml_sync_values(bml);
  bml(tick(bm));

  /* don't process data in passthrough-mode
  if (gst_base_transform_is_passthrough (base))
    return GST_FLOW_OK;
  */

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET(outbuf,GST_BUFFER_FLAG_GAP)) {
    mode=2; /* WM_WRITE */
  }
  else {
    //for(i=0;i<samples_per_buffer;i++) datai[i]*=32768.0;
    gfloat fc=32768.0;
    oil_scalarmultiply_f32_ns (datai, datai, &fc, samples_per_buffer);
  }

  GST_DEBUG("  calling work_m2s(%d,%d)",samples_per_buffer,mode);
  todo = samples_per_buffer;
  seg_datai = datai;
  seg_datao = datao;
  has_data = FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo>256) ? 256 : todo; 
    has_data |= bml(work_m2s(bm,seg_datai,seg_datao,(int)seg_size,mode));
    seg_datai = &seg_datai[seg_size];
    seg_datao = &seg_datao[seg_size*2];
    todo -= seg_size;
  }
  if(!has_data) {
    GST_INFO_OBJECT(bml_transform,"silent buffer");
    GST_BUFFER_FLAG_SET(outbuf,GST_BUFFER_FLAG_GAP);
  }
  else {
    GST_BUFFER_FLAG_UNSET(outbuf,GST_BUFFER_FLAG_GAP);
    //for(i=0;i<samples_per_buffer;i++) datao[i]/=32768.0;
    gfloat fc=1.0/32768.0;
    oil_scalarmultiply_f32_ns (datao, datao, &fc, samples_per_buffer);
  }

  return(GST_FLOW_OK);
}

static void gst_bml_transform_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(object);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBML *bml=GST_BML(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  bml(gstbml_set_property(bml,bml_class,prop_id,value,pspec));
}

static void gst_bml_transform_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(object);
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBML *bml=GST_BML(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  bml(gstbml_get_property(bml,bml_class,prop_id,value,pspec));
}

static void gst_bml_transform_dispose(GObject *object) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(object);
  GstBML *bml=GST_BML(bml_transform);

  gstbml_dispose(bml);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void gst_bml_transform_finalize(GObject *object) {
  GstBMLTransform *bml_transform=GST_BML_TRANSFORM(object);
  GstBML *bml=GST_BML(bml_transform);

  bml(gstbml_finalize(bml));

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void gst_bml_transform_base_finalize(GstBMLTransformClass *klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  gstbml_preset_finalize(bml_class);
}

static void gst_bml_transform_init(GstBMLTransform *bml_transform) {
  GstBMLTransformClass *klass=GST_BML_TRANSFORM_GET_CLASS(bml_transform);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GstBML *bml=GST_BML(bml_transform);

  GST_INFO("initializing instance: elem=%p, bml=%p, bml_class=%p, size=%d",bml_transform,bml,bml_class,sizeof (GstBMLTransform));
  GST_INFO("bm=0x%p, src=%d, sink=%d",bml_class->bm,bml_class->numsrcpads,bml_class->numsinkpads);

  bml(gstbml_init(bml,bml_class,GST_ELEMENT(bml_transform)));
  /* this is not nedded when using the base class
  bml(gstbml_init_pads(GST_ELEMENT(bml_transform),bml,gst_bml_transform_link));

  if (sinkcount == 1) {
    // with one sink (input ports) we can use the chain function
    // effects
    GST_DEBUG_OBJECT(bml, "chain mode");
    gst_pad_set_chain_function(bml->sinkpads[0], gst_bml_transform_chain);
  }
  else if (sinkcount > 1) {
    // more than one sink (input ports) pad needs loop mode
    // auxbus based effects
    GST_DEBUG_OBJECT(bml, "loop mode with %d sink pads and %d src pads", sinkcount, srccount);
    gst_element_set_loop_function(GST_ELEMENT(bml_transform), gst_bml_transform_loop);
  }
  */

#if GST_CHECK_VERSION(0,10,17)
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (bml_transform), TRUE);
#endif

  GST_DEBUG("  done");
}

static void gst_bml_transform_class_init(GstBMLTransformClass *klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GObjectClass *gobject_class=G_OBJECT_CLASS(klass);
  GstBaseTransformClass *gstbasetransform_class=GST_BASE_TRANSFORM_CLASS(klass);

  GST_INFO("initializing class");
  parent_class=g_type_class_peek_parent(klass);

  // override methods
  gobject_class->set_property          = GST_DEBUG_FUNCPTR(gst_bml_transform_set_property);
  gobject_class->get_property          = GST_DEBUG_FUNCPTR(gst_bml_transform_get_property);
  gobject_class->dispose               = GST_DEBUG_FUNCPTR(gst_bml_transform_dispose);
  gobject_class->finalize              = GST_DEBUG_FUNCPTR(gst_bml_transform_finalize);
  gstbasetransform_class->set_caps     = GST_DEBUG_FUNCPTR(gst_bml_transform_set_caps);
  gstbasetransform_class->stop         = GST_DEBUG_FUNCPTR(gst_bml_transform_stop);
  if(bml_class->output_channels==1) {
    gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_bml_transform_transform_ip_mono);
  }
  else {
    if(bml_class->input_channels==1) {
      gstbasetransform_class->transform = GST_DEBUG_FUNCPTR(gst_bml_transform_transform_mono_to_stereo);
      gstbasetransform_class->get_unit_size = GST_DEBUG_FUNCPTR(gst_bml_transform_get_unit_size);
      gstbasetransform_class->transform_caps = GST_DEBUG_FUNCPTR (gst_bml_transform_transform_caps);
    }
    else {
      gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_bml_transform_transform_ip_stereo);
    }
  }

  // override interface properties and register parameters as gobject properties
  bml(gstbml_class_prepare_properties(gobject_class,bml_class));
}

static void gst_bml_transform_base_init(GstBMLTransformClass *klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GstElementClass *element_class=GST_ELEMENT_CLASS(klass);
  //GstPadTemplate *templ;
  gpointer bm;

  GST_INFO("initializing base");

  bm=bml(gstbml_class_base_init(bml_class,G_TYPE_FROM_CLASS(klass),1,1));

  if(bml_class->output_channels==1) {
    gst_element_class_add_pad_template(element_class,gst_static_pad_template_get(&bml_pad_caps_mono_src_template));
    GST_INFO("  added mono src pad template");
  }
  else {
    gst_element_class_add_pad_template(element_class,gst_static_pad_template_get(&bml_pad_caps_stereo_src_template));
    GST_INFO("  added stereo src pad template");
  }
  if(bml_class->output_channels==1) {
    gst_element_class_add_pad_template(element_class,gst_static_pad_template_get(&bml_pad_caps_mono_sink_template));
    GST_INFO("  added mono sink pad template");
  }
  else {
    gst_element_class_add_pad_template(element_class,gst_static_pad_template_get(&bml_pad_caps_stereo_sink_template));
    GST_INFO("  added stereo sink pad template");
  }

  bml(gstbml_class_set_details(element_class,bm,"Filter/Effect/Audio/BML"));
}


GType bml(transform_get_type(const char *element_type_name, gpointer bm)) {
  const GTypeInfo element_type_info = {
    sizeof (GstBMLTransformClass),
    (GBaseInitFunc) gst_bml_transform_base_init,
    (GBaseFinalizeFunc) gst_bml_transform_base_finalize,
    (GClassInitFunc) gst_bml_transform_class_init,
    NULL,
    NULL,
    sizeof (GstBMLTransform),
    0,
    (GInstanceInitFunc) gst_bml_transform_init,
  };
  const GInterfaceInfo child_proxy_interface_info = {
    (GInterfaceInitFunc) gst_bml_child_proxy_interface_init,    /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo child_bin_interface_info = {
    NULL,               /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo property_meta_interface_info = {
    (GInterfaceInitFunc) gst_bml_property_meta_interface_init,  /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo tempo_interface_info = {
    (GInterfaceInitFunc) gst_bml_tempo_interface_init,          /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo help_interface_info = {
    NULL,               /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo preset_interface_info = {
    (GInterfaceInitFunc) gst_bml_preset_interface_init,        /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  GType element_type;

  GST_INFO("registering transform-plugin 0x%p: \"%s\"",bm,element_type_name);

  g_assert(bm);

  // create the element type now
  element_type = g_type_register_static(GST_TYPE_BASE_TRANSFORM, element_type_name, &element_type_info, 0);
  GST_INFO("succefully registered new type : \"%s\"", element_type_name);
  g_type_add_interface_static(element_type, GST_TYPE_PROPERTY_META, &property_meta_interface_info);
  g_type_add_interface_static(element_type, GST_TYPE_TEMPO, &tempo_interface_info);

  // check if this plugin can do multiple voices
  if(bml(gstbml_is_polyphonic(bm))) {
    g_type_add_interface_static(element_type, GST_TYPE_CHILD_PROXY, &child_proxy_interface_info);
    g_type_add_interface_static(element_type, GST_TYPE_CHILD_BIN, &child_bin_interface_info);
  }
  // check if this plugin has user docs
  if(gstbml_get_help_uri(bm)) {
    g_type_add_interface_static(element_type, GST_TYPE_HELP, &help_interface_info);
  }
  // add presets iface
  g_type_add_interface_static(element_type, GST_TYPE_PRESET, &preset_interface_info);

  GST_INFO("succefully registered type interfaces");

  return(element_type);
}
