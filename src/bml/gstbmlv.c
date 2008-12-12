/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at users.sf.net>
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

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);

static GstElementClass *parent_class = NULL;

extern GHashTable *bml_descriptors_by_voice_type;

//-- local helper


//-- property meta interface implementations

static gchar *bml(v_property_meta_describe_property(gpointer bm, glong index, GValue *event)) {
  const gchar *str=NULL;
  gchar def[20];
  GType base,type=G_VALUE_TYPE(event);

  while((base=g_type_parent(type))) type=base;

  switch(type) {
    case G_TYPE_INT:
      if(!(str=bml(describe_track_value(bm, index, g_value_get_int(event)))) || !*str) {
        sprintf(def,"%d",g_value_get_int(event));
        str=def;
      }
      break;
    case G_TYPE_UINT:
      if(!(str=bml(describe_track_value(bm, index, (gint)g_value_get_uint(event)))) || !*str) {
        sprintf(def,"%u",g_value_get_uint(event));
        str=def;
      }
      break;
    case G_TYPE_ENUM:
      if(!(str=bml(describe_track_value(bm, index, g_value_get_enum(event)))) || !*str) {
        // @todo: get blurb for enum value
        sprintf(def,"%d",g_value_get_enum(event));
        str=def;
      }
      break;
    default:
      GST_ERROR("unsupported GType=%d:'%s'",G_VALUE_TYPE(event),G_VALUE_TYPE_NAME(event));
      return(g_strdup_value_contents(event));
  }
  GST_INFO("formatted track parameter : '%s'",str);
  return(g_strdup(str));
}

static gchar *gst_bmlv_property_meta_describe_property(GstBtPropertyMeta *property_meta, glong index, GValue *event) {
  GstBMLV *bmlv=GST_BMLV(property_meta);

  return(bml(v_property_meta_describe_property(bmlv->bm,index,event)));
}

static void gst_bmlv_property_meta_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtPropertyMetaInterface *iface = g_iface;

  GST_INFO("initializing iface");

  iface->describe_property = gst_bmlv_property_meta_describe_property;
}


//-- gstbmlvoice class implementation

static void gst_bmlv_set_property(GObject *object, guint prop_id, const GValue * value, GParamSpec * pspec) {
  GstBMLV *bmlv=GST_BMLV(object);
  gpointer bm=bmlv->bm;
  gint *addr;
  gint type;

  // property ids have an offset of 1
  prop_id--;

  addr=(gint *)bml(get_track_parameter_location(bm,bmlv->voice,prop_id));
  type=GPOINTER_TO_INT(g_param_spec_get_qdata(pspec,gst_bml_property_meta_quark_type));
  // @todo cache this info
  //bml(get_track_parameter_info(bm,prop_id,BM_PARA_TYPE,(void *)&type));
  gstbml_set_param(type,addr,value);

  /*{ DEBUG
    gchar *valstr=g_strdup_value_contents(value);
    GST_DEBUG("set track param %d:%d to %s", prop_id, bmlv->voice, valstr);
    g_free(valstr);
  } DEBUG */
}

static void gst_bmlv_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
  GstBMLV *bmlv=GST_BMLV(object);
  gpointer bm=bmlv->bm;
  gint *addr;
  gint type;

  // property ids have an offset of 1
  prop_id--;

  addr=(gint *)bml(get_track_parameter_location(bm,bmlv->voice,prop_id));
  type=GPOINTER_TO_INT(g_param_spec_get_qdata(pspec,gst_bml_property_meta_quark_type));
  // @todo cache this info
  //bml(get_track_parameter_info(bm,prop_id,BM_PARA_TYPE,(void *)&type));
  gstbml_get_param(type,addr,value);
  /*{ DEBUG
    gchar *valstr=g_strdup_value_contents(value);
    GST_DEBUG("got track param %d:%d as %s", prop_id, bmlv->voice, valstr);
    g_free(valstr);
  } DEBUG */
}

static void gst_bmlv_dispose(GObject *object) {
  GstBMLV *bmlv=GST_BMLV(object);

  if(bmlv->dispose_has_run) return;
  bmlv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! bmlv=%p",bmlv);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void gst_bmlv_finalize(GObject *object) {
  GstBMLV *bmlv=GST_BMLV(object);

  GST_DEBUG("!!!! bmlv=%p",bmlv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void gst_bmlv_base_init(GstBMLVClass *klass) {
  gpointer bm;
  GST_INFO("initializing base");

  bm=g_hash_table_lookup(bml_descriptors_by_voice_type,GINT_TO_POINTER(G_TYPE_FROM_CLASS(klass)));
  if(!bm) bm=g_hash_table_lookup(bml_descriptors_by_voice_type,GINT_TO_POINTER(0));
  g_assert(bm);

  GST_INFO("  bm=0x%p",bm);
  klass->bm = bm;
}

static void gst_bmlv_class_init(GstBMLVClass *klass) {
  gpointer bm;
  GType enum_type = 0;
  GObjectClass *gobject_class;
  gint num;

  GST_INFO("initializing class");
  bm=klass->bm;

  parent_class=g_type_class_peek_parent(klass);

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = gst_bmlv_set_property;
  gobject_class->get_property = gst_bmlv_get_property;
  gobject_class->dispose      = gst_bmlv_dispose;
  gobject_class->finalize     = gst_bmlv_finalize;

  if(bml(get_machine_info(bm,BM_PROP_NUM_TRACK_PARAMS,(void *)&num))) {
    gint min_val,max_val,def_val,no_val;
    gint type,flags;
    gchar *tmp_name,*tmp_desc;
    gchar *name,*nick,*desc;
    gint i, prop_id=1;

    //klass->numtrackparams=num;
    GST_INFO("  machine has %d track params ",num);
    for(i=0;i<num;i++,prop_id++) {
      GST_DEBUG("      track_param=%02i",i);
      if(bml(get_track_parameter_info(bm,i,BM_PARA_TYPE,(void *)&type)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_NAME,(void *)&tmp_name)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_DESCRIPTION,(void *)&tmp_desc)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_FLAGS,(void *)&flags)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_MIN_VALUE,(void *)&min_val)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_MAX_VALUE,(void *)&max_val)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_NO_VALUE,(void *)&no_val)) &&
        bml(get_track_parameter_info(bm,i,BM_PARA_DEF_VALUE,(void *)&def_val))
      ) {
        gstbml_convert_names(gobject_class, tmp_name, tmp_desc, &name, &nick, &desc);
        // create an enum on the fly
        if(type==PT_BYTE) {
          if((enum_type = bml(gstbml_register_track_enum_type(gobject_class, bm, i, name, min_val, max_val, no_val)))) {
            type = PT_ENUM;
          }
        }

        if(gstbml_register_param(gobject_class, prop_id, type, enum_type, name, nick, desc, flags, min_val, max_val, no_val, def_val)) {
          klass->numtrackparams++;
        }
        else {
          GST_WARNING("registering voice_param failed!");
        }
        g_free(name);g_free(nick);g_free(desc);
      }
    }
  }
  GST_INFO("  %d track params installed",klass->numtrackparams);
}

static void gst_bmlv_init(GstBMLV *bmlv) {
  GST_INFO("initializing instance");
}

//--

GType bml(v_get_type(gchar *voice_type_name)) {
  GType type;
  const GTypeInfo voice_type_info = {
    sizeof (GstBMLVClass),
    (GBaseInitFunc) gst_bmlv_base_init,
    NULL,
    (GClassInitFunc) gst_bmlv_class_init,
    NULL,
    NULL,
    sizeof (GstBMLV),
    0,
    (GInstanceInitFunc) gst_bmlv_init,
  };
  const GInterfaceInfo property_meta_interface_info = {
    (GInterfaceInitFunc) gst_bmlv_property_meta_interface_init,  /* interface_init */
    NULL,               /* interface_finalize */
    NULL                /* interface_data */
  };
  // @todo should we have a hashmap by voice_type_name ?
  type = g_type_register_static(GST_TYPE_OBJECT, voice_type_name, &voice_type_info, 0);
  g_type_add_interface_static(type, GSTBT_TYPE_PROPERTY_META, &property_meta_interface_info);
  return(type);
}
