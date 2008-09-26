/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * gstbmlsrc.c: BML source plugin
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

// see also: gstfakesrc, gstfilesrc, gsttestaudiosrc

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

static GstBaseSrcClass *parent_class = NULL;

extern GHashTable *bml_descriptors_by_element_type;
extern GHashTable *bml_descriptors_by_voice_type;
extern GType voice_type;

//-- child bin interface implementations


//-- child proxy interface implementations

static GstObject *gst_bml_child_proxy_get_child_by_index(GstChildProxy *child_proxy, guint index) {
  GstBMLSrc *bml_src=GST_BML_SRC(child_proxy);
  GstBML *bml=GST_BML(bml_src);

  g_return_val_if_fail(index<bml->num_voices,NULL);

  return(g_object_ref(g_list_nth_data(bml->voices,index)));
}

static guint gst_bml_child_proxy_get_children_count(GstChildProxy *child_proxy) {
  GstBMLSrc *bml_src=GST_BML_SRC(child_proxy);
  GstBML *bml=GST_BML(bml_src);

  return(bml->num_voices);
}


static void gst_bml_child_proxy_interface_init(gpointer g_iface, gpointer iface_data) {
  GstChildProxyInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->get_child_by_index = gst_bml_child_proxy_get_child_by_index;
  iface->get_children_count = gst_bml_child_proxy_get_children_count;
}

//-- property meta interface implementations

static gchar *gst_bml_property_meta_describe_property(GstBtPropertyMeta *property_meta, glong index, GValue *event) {
  GstBML *bml=GST_BML(GST_BML_SRC(property_meta));

  return(bml(gstbml_property_meta_describe_property(bml->bm,index,event)));
}

static void gst_bml_property_meta_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtPropertyMetaInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->describe_property = gst_bml_property_meta_describe_property;
}

//-- tempo interface implementations

static void gst_bml_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  GstBMLSrc *bml_src=GST_BML_SRC(tempo);
  GstBML *bml=GST_BML(bml_src);

  bml(gstbml_tempo_change_tempo(G_OBJECT(bml_src),bml,beats_per_minute,ticks_per_beat,subticks_per_tick));
}

static void gst_bml_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->change_tempo = gst_bml_tempo_change_tempo;
}

//-- preset interface implementations

static gchar** gst_bml_preset_get_preset_names(GstPreset *preset) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBML *bml=GST_BML(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_get_preset_names(bml,bml_class));
}

static gboolean gst_bml_preset_load_preset(GstPreset *preset, const gchar *name) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBML *bml=GST_BML(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_load_preset(GST_OBJECT(preset),bml,bml_class,name));
}

static gboolean gst_bml_preset_save_preset(GstPreset *preset, const gchar *name) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBML *bml=GST_BML(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_save_preset(GST_OBJECT(preset),bml,bml_class,name));
}

static gboolean gst_bml_preset_rename_preset(GstPreset *preset, const gchar *old_name, const gchar *new_name) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_rename_preset(bml_class,old_name,new_name));
}

static gboolean gst_bml_preset_delete_preset(GstPreset *preset, const gchar *name) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_delete_preset(bml_class,name));
}

static gboolean gst_bml_set_meta (GstPreset *preset,const gchar *name, const gchar *tag, const gchar *value) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  return(gstbml_preset_set_meta(bml_class,name,tag,value));
}

static gboolean gst_bml_get_meta (GstPreset *preset,const gchar *name, const gchar *tag, gchar **value) {
  GstBMLSrc *bml_src=GST_BML_SRC(preset);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
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

//-- gstbmlsrc class implementation

static void gst_bml_src_src_fixate(GstPad *pad, GstCaps *caps) {
  GstBMLSrc *bml_src=GST_BML_SRC(GST_PAD_PARENT(pad));
  GstBML *bml=GST_BML(bml_src);
  GstStructure *structure;

  GST_INFO_OBJECT (bml_src, "setting sample rate to %d", bml->samplerate);

  structure=gst_caps_get_structure(caps, 0);
  gst_structure_fixate_field_nearest_int(structure, "rate", bml->samplerate);
}

static gboolean gst_bml_src_set_caps(GstBaseSrc *base, GstCaps *caps) {
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  GstStructure *structure;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  if((ret = gst_structure_get_int(structure, "rate", &bml->samplerate))) {
    bml(set_master_info(bml->beats_per_minute,bml->ticks_per_beat,bml->samplerate));
    bml(init(bml->bm,0,NULL));
  }
  
  return ret;
}

static gboolean gst_bml_src_query(GstBaseSrc *base, GstQuery * query) {
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_DEFAULT:
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val = gst_util_uint64_scale_int (src_val, GST_SECOND,
                bml->samplerate);
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val = gst_util_uint64_scale_int (src_val, bml->samplerate,
                GST_SECOND);
              break;
            default:
              goto error;
          }
          break;
        default:
          goto error;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      res = TRUE;
      break;
    }
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (base, query);
      break;
  }

  return res;
  /* ERROR */
error:
  {
    GST_DEBUG_OBJECT (bml, "query failed");
    return FALSE;
  }
}

static void gst_bml_src_get_times(GstBaseSrc * base, GstBuffer * buffer, GstClockTime * start, GstClockTime * end) {
  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live (base)) {
    GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* get duration to calculate end time */
      GstClockTime duration = GST_BUFFER_DURATION (buffer);

      if (GST_CLOCK_TIME_IS_VALID (duration)) {
        *end = timestamp + duration;
      }
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

static gboolean gst_bml_src_stop(GstBaseSrc * base) {
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  gpointer bm=bml->bm;
  
  bml(stop(bm));
  return TRUE;
}

static gboolean gst_bml_src_do_seek(GstBaseSrc * base, GstSegment * segment) {
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  GstClockTime time;

  segment->time = segment->start;
  time = segment->last_stop;

  /* now move to the time indicated */
  bml->n_samples = gst_util_uint64_scale_int(time, bml->samplerate, GST_SECOND);
  bml->running_time = time;

  g_assert (bml->running_time <= time);

  if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
    time = segment->stop;
    bml->n_samples_stop = gst_util_uint64_scale_int(time, bml->samplerate,
        GST_SECOND);
    bml->check_seek_stop = TRUE;
  } else {
    bml->check_seek_stop = FALSE;
  }
  bml->seek_flags = segment->flags;
  bml->eos_reached = FALSE;

  return TRUE;
}

static gboolean gst_bml_src_is_seekable(GstBaseSrc * base) {
  /* we're seekable... */
  return TRUE;
}

static GstFlowReturn gst_bml_src_create_mono(GstBaseSrc *base, GstClockTime offset, guint length, GstBuffer ** buffer) {
  GstFlowReturn res;
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples;
  gdouble samples_done;
  GstPad *srcpad=GST_BASE_SRC_PAD(base);
  BMLData *data,*seg_data;
  gpointer bm=bml->bm;
  guint todo,seg_size,samples_per_buffer;
  gboolean has_data;

  if (G_UNLIKELY(bml->eos_reached)) {
    GST_DEBUG("  EOS reached");
    return GST_FLOW_UNEXPECTED;
  }

  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done = (gdouble)bml->running_time*(gdouble)bml->samplerate/(gdouble)GST_SECOND;
  samples_per_buffer=(guint)(bml->samples_per_buffer+(samples_done-(gdouble)bml->n_samples));

  /* check for eos */
  if (bml->check_seek_stop &&
    (bml->n_samples_stop > bml->n_samples) &&
    (bml->n_samples_stop < bml->n_samples + samples_per_buffer)
  ) {
    /* calculate only partial buffer */
    samples_per_buffer = bml->n_samples_stop - bml->n_samples;
    n_samples = bml->n_samples_stop;
    if (!(bml->seek_flags&GST_SEEK_FLAG_SEGMENT)) {
      bml->eos_reached = TRUE;
    }
  } else {
    /* calculate full buffer */
    n_samples = bml->n_samples + samples_per_buffer;
  }

  next_time = gst_util_uint64_scale(n_samples,GST_SECOND,(guint64)bml->samplerate);

  /* allocate a new buffer suitable for this pad */
  if ((res = gst_pad_alloc_buffer_and_set_caps (srcpad, bml->n_samples,
      samples_per_buffer * sizeof(BMLData),
      GST_PAD_CAPS (srcpad),
      &buf)) != GST_FLOW_OK
  ) {
    return res;
  }

  GST_BUFFER_TIMESTAMP(buf)=bml->running_time;
  GST_BUFFER_OFFSET_END(buf)=n_samples;
  GST_BUFFER_DURATION(buf)=next_time - bml->running_time;

  /* @todo: split up processing of the buffer into chunks so that params can
   * be updated when required (e.g. for the subticks-feature).
   */
  gstbml_sync_values(bml);
  bml(tick(bm));

  bml->running_time += bml->ticktime;
  //bml->running_time = next_time;
  bml->n_samples = n_samples;

  GST_DEBUG("  calling work(%d)",samples_per_buffer);
  data = (BMLData *)GST_BUFFER_DATA(buf);
  // some buzzmachines expect a cleared buffer
  //for(i=0;i<samples_per_buffer;i++) data[i]=0.0f;
  memset(data,0,samples_per_buffer*sizeof(BMLData));

  todo = samples_per_buffer;
  seg_data = data;
  has_data=FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo>256) ? 256 : todo; 
    /* mode does not really matter for generators */
    has_data |= bml(work(bm,seg_data,(int)seg_size,2/*WM_WRITE*/));
    seg_data = &seg_data[seg_size];
    todo -= seg_size;
  }
  if(!has_data) {
    GST_INFO_OBJECT(bml_src,"silent buffer");
    GST_BUFFER_FLAG_SET(buf,GST_BUFFER_FLAG_GAP);
  }

  // buzz generates relative loud output
  //guint i;
  //for(i=0;i<samples_per_buffer;i++) data[i]/=32768.0f;
  gfloat fc=1.0/32768.0;
  oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer);

  // return results
  *buffer = buf;

  return(GST_FLOW_OK);
}

static GstFlowReturn gst_bml_src_create_stereo(GstBaseSrc *base, GstClockTime offset, guint length, GstBuffer ** buffer) {
  GstFlowReturn res;
  GstBMLSrc *bml_src=GST_BML_SRC(base);
  GstBML *bml=GST_BML(bml_src);
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 n_samples;
  gdouble samples_done;
  GstPad *srcpad=GST_BASE_SRC_PAD(base);
  BMLData *data,*seg_data;
  gpointer bm=bml->bm;
  guint todo,seg_size,samples_per_buffer;
  gboolean has_data;

  if (G_UNLIKELY(bml->eos_reached)) {
    GST_DEBUG("  EOS reached");
    return GST_FLOW_UNEXPECTED;
  }

  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done = (gdouble)bml->running_time*(gdouble)bml->samplerate/(gdouble)GST_SECOND;
  samples_per_buffer=(guint)(bml->samples_per_buffer+(samples_done-(gdouble)bml->n_samples));

  /* check for eos */
  if (bml->check_seek_stop &&
    (bml->n_samples_stop > bml->n_samples) &&
    (bml->n_samples_stop < bml->n_samples + samples_per_buffer)
  ) {
    /* calculate only partial buffer */
    samples_per_buffer = bml->n_samples_stop - bml->n_samples;
    n_samples = bml->n_samples_stop;
    bml->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    n_samples = bml->n_samples + samples_per_buffer;
  }

  next_time = gst_util_uint64_scale(n_samples,GST_SECOND,(guint64)bml->samplerate);

  /* allocate a new buffer suitable for this pad */
  if ((res = gst_pad_alloc_buffer_and_set_caps (srcpad, bml->n_samples,
      samples_per_buffer * 2 * sizeof(BMLData),
      GST_PAD_CAPS (srcpad),
      &buf)) != GST_FLOW_OK
  ) {
    return res;
  }

  GST_BUFFER_TIMESTAMP(buf)=bml->running_time;
  GST_BUFFER_OFFSET_END(buf)=n_samples;
  GST_BUFFER_DURATION(buf)=next_time - bml->running_time;

  /* @todo: split up processing of the buffer into chunks so that params can
   * be updated when required (e.g. for the subticks-feature).
   */
  gstbml_sync_values(bml);
  bml(tick(bm));

  bml->running_time += bml->ticktime;
  //bml->running_time = next_time;
  bml->n_samples = n_samples;

  GST_DEBUG("  calling work_m2s(%d)",samples_per_buffer);
  data=(BMLData *)GST_BUFFER_DATA(buf);
  // some buzzmachines expect a cleared buffer
  //for(i=0;i<samples_per_buffer*2;i++) data[i]=0.0;
  memset(data,0,samples_per_buffer*2*sizeof(BMLData));

  todo = samples_per_buffer;
  seg_data = data;
  has_data=FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo>256) ? 256 : todo; 
    /* mode does not really matter for generators, NULL for input should be fine */
    has_data |= bml(work_m2s(bm,NULL,seg_data,(int)seg_size,2/*WM_WRITE*/));
    seg_data = &seg_data[seg_size*2];
    todo -= seg_size;
  }
  if(!has_data) {
    GST_INFO_OBJECT(bml_src,"silent buffer");
    GST_BUFFER_FLAG_SET(buf,GST_BUFFER_FLAG_GAP);
  }

  // buzz generates relative loud output
  //guint i;
  //for(i=0;i<samples_per_buffer*2;i++) data[i]/=32768.0;
  gfloat fc=1.0/32768.0;
  oil_scalarmultiply_f32_ns (data, data, &fc, samples_per_buffer*2);

  // return results
  *buffer = buf;

  return(GST_FLOW_OK);
}

static void gst_bml_src_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
  GstBMLSrc *bml_src=GST_BML_SRC(object);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBML *bml=GST_BML(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  bml(gstbml_set_property(bml,bml_class,prop_id,value,pspec));
}

static void gst_bml_src_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
  GstBMLSrc *bml_src=GST_BML_SRC(object);
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBML *bml=GST_BML(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  bml(gstbml_get_property(bml,bml_class,prop_id,value,pspec));
}

static void gst_bml_src_dispose(GObject *object) {
  GstBMLSrc *bml_src=GST_BML_SRC(object);
  GstBML *bml=GST_BML(bml_src);

  gstbml_dispose(bml);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void gst_bml_src_finalize(GObject *object) {
  GstBMLSrc *bml_src=GST_BML_SRC(object);
  GstBML *bml=GST_BML(bml_src);

  bml(gstbml_finalize(bml));

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void gst_bml_src_base_finalize(GstBMLSrcClass *klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);

  gstbml_preset_finalize(bml_class);
}

static void gst_bml_src_init(GstBMLSrc *bml_src) {
  GstBMLSrcClass *klass=GST_BML_SRC_GET_CLASS(bml_src);
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GstBML *bml=GST_BML(bml_src);
  GstPad *srcpad;

  GST_INFO("initializing instance: elem=%p, bml=%p, bml_class=%p, size=%d",bml_src,bml,bml_class,sizeof (GstBMLSrc));
  GST_INFO("bm=0x%p, src=%d, sink=%d",bml_class->bm,bml_class->numsrcpads,bml_class->numsinkpads);

  bml(gstbml_init(bml,bml_class,GST_ELEMENT(bml_src)));
  /* we operate in time */
  gst_base_src_set_format(GST_BASE_SRC (bml_src), GST_FORMAT_TIME);
  gst_base_src_set_live(GST_BASE_SRC(bml_src), FALSE);

  //bml(gstbml_pads(GST_ELEMENT(bml_src),bml,gst_bml_src_link));
  srcpad=GST_BASE_SRC_PAD(bml_src);
  gst_pad_set_fixatecaps_function (srcpad, gst_bml_src_src_fixate);

  GST_DEBUG("  done");
}

static void gst_bml_src_class_init(GstBMLSrcClass * klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GObjectClass *gobject_class=G_OBJECT_CLASS(klass);
  GstBaseSrcClass *gstbasesrc_class=GST_BASE_SRC_CLASS(klass);

  GST_INFO("initializing class: size=%d",sizeof (GstBMLSrcClass));
  parent_class=g_type_class_peek_parent(klass);

  // override methods
  gobject_class->set_property   = GST_DEBUG_FUNCPTR(gst_bml_src_set_property);
  gobject_class->get_property   = GST_DEBUG_FUNCPTR(gst_bml_src_get_property);
  gobject_class->dispose        = GST_DEBUG_FUNCPTR(gst_bml_src_dispose);
  gobject_class->finalize       = GST_DEBUG_FUNCPTR(gst_bml_src_finalize);
  gstbasesrc_class->set_caps    = GST_DEBUG_FUNCPTR(gst_bml_src_set_caps);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR(gst_bml_src_is_seekable);
  gstbasesrc_class->do_seek     = GST_DEBUG_FUNCPTR(gst_bml_src_do_seek);
  gstbasesrc_class->query       = GST_DEBUG_FUNCPTR(gst_bml_src_query);
  gstbasesrc_class->get_times   = GST_DEBUG_FUNCPTR(gst_bml_src_get_times);
  gstbasesrc_class->stop        = GST_DEBUG_FUNCPTR(gst_bml_src_stop);
  if(bml_class->output_channels==1) {
    gstbasesrc_class->create    = GST_DEBUG_FUNCPTR(gst_bml_src_create_mono);
  }
  else {
    gstbasesrc_class->create    = GST_DEBUG_FUNCPTR(gst_bml_src_create_stereo);
  }

  // override interface properties and register parameters as gobject properties
  bml(gstbml_class_prepare_properties(gobject_class,bml_class));
}

static void gst_bml_src_base_init(GstBMLSrcClass *klass) {
  GstBMLClass *bml_class=GST_BML_CLASS(klass);
  GstElementClass *element_class=GST_ELEMENT_CLASS(klass);
  //GstPadTemplate *templ;
  gpointer bm;

  GST_INFO("initializing base");

  bm=bml(gstbml_class_base_init(bml_class,G_TYPE_FROM_CLASS(klass),1,0));

  if(bml_class->output_channels==1) {
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get (&bml_pad_caps_mono_src_template));
    GST_INFO("  added mono src pad template");
  }
  else {
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get (&bml_pad_caps_stereo_src_template));
    GST_INFO("  added stereo src pad template");
  }

  bml(gstbml_class_set_details(element_class,bm,"Source/Audio/BML"));
}


GType bml(src_get_type(const char *element_type_name, gpointer bm)) {
  const GTypeInfo element_type_info = {
    sizeof (GstBMLSrcClass),
    (GBaseInitFunc) gst_bml_src_base_init,
    (GBaseFinalizeFunc) gst_bml_src_base_finalize,
    (GClassInitFunc) gst_bml_src_class_init,
    NULL,
    NULL,
    sizeof (GstBMLSrc),
    0,
    (GInstanceInitFunc) gst_bml_src_init,
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
    (GInterfaceInitFunc) gst_bml_property_meta_interface_init, /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  const GInterfaceInfo tempo_interface_info = {
    (GInterfaceInitFunc) gst_bml_tempo_interface_init,         /* interface_init */
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

  GST_INFO("registering source-plugin 0x%p: \"%s\"",bm,element_type_name);

  g_assert(bm);

  // create the element type now
  element_type = g_type_register_static(GST_TYPE_BASE_SRC, element_type_name, &element_type_info, 0);
  GST_INFO("succefully registered new type : \"%s\"", element_type_name);
  g_type_add_interface_static(element_type, GSTBT_TYPE_PROPERTY_META, &property_meta_interface_info);
  g_type_add_interface_static(element_type, GSTBT_TYPE_TEMPO, &tempo_interface_info);

  // check if this plugin can do multiple voices
  if(bml(gstbml_is_polyphonic(bm))) {
    g_type_add_interface_static(element_type, GST_TYPE_CHILD_PROXY, &child_proxy_interface_info);
    g_type_add_interface_static(element_type, GSTBT_TYPE_CHILD_BIN, &child_bin_interface_info);
  }
  // check if this plugin has user docs
  if(gstbml_get_help_uri(bm)) {
    g_type_add_interface_static(element_type, GSTBT_TYPE_HELP, &help_interface_info);
  }
  // add presets iface
  g_type_add_interface_static(element_type, GST_TYPE_PRESET, &preset_interface_info);

  GST_INFO("succefully registered type interfaces");

  return(element_type);
}
