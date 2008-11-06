/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * utils.c: Functions shared among source and transform elements
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

extern GType voice_type;
extern GHashTable *bml_descriptors_by_element_type;
extern GHashTable *bml_help_uri_by_descriptor;
extern GHashTable *bml_preset_path_by_descriptor;
extern GHashTable *bml_dllpath_by_element_type;

//-- helper

/*
 * bml_is_polyphonic:
 *
 * Test if a buzzmachine supports voices.
 *
 * Returns: %TRUE if the machine is polyphonic
 */
gboolean bml(gstbml_is_polyphonic(gpointer bm)) {
  int track_params=0;
  //int min_voices=0,max_voices=0;

  if(bml(get_machine_info(bm,BM_PROP_NUM_TRACK_PARAMS,(void *)&track_params))) {
    if(track_params>0) return(TRUE);
#if 0
  if(bml(bml_get_machine_info(bm,BM_PROP_MIN_TRACKS,(void *)&min_voices)) &&
    bml(bml_get_machine_info(bm,BM_PROP_MAX_TRACKS,(void *)&max_voices))
  ) {
    GST_INFO("min/max-voices=%d/%d",min_voices,max_voices);
    if((min_voices<=max_voices) && (max_voices>0)) return(TRUE);
  }
#endif
  }
  return(FALSE);
}


//-- common iface functions

gchar *bml(gstbml_property_meta_describe_property(gpointer bm, glong index, GValue *event)) {
  const gchar *str=NULL;
  gchar def[20];
  GType base,type=G_VALUE_TYPE(event);

  while((base=g_type_parent(type))) type=base;

  switch(type) {
    case G_TYPE_INT:
      if(!(str=bml(describe_global_value(bm, index, g_value_get_int(event)))) || !*str) {
        sprintf(def,"%d",g_value_get_int(event));
        str=def;
      }
      break;
    case G_TYPE_UINT:
      if(!(str=bml(describe_global_value(bm, index, (gint)g_value_get_uint(event)))) || !*str) {
        sprintf(def,"%u",g_value_get_uint(event));
        str=def;
      }
      break;
    case G_TYPE_ENUM:
      if(!(str=bml(describe_global_value(bm, index, g_value_get_enum(event)))) || !*str) {
        // @todo: get blurb for enum value
        sprintf(def,"%d",g_value_get_enum(event));
        str=def;
      }
      break;
    default:
      GST_ERROR("unsupported GType=%d:'%s'",G_VALUE_TYPE(event),G_VALUE_TYPE_NAME(event));
      return(g_strdup_value_contents(event));
  }
  GST_INFO("formatted global parameter : '%s'",str);
  return(g_strdup(str));
}

void bml(gstbml_tempo_change_tempo(GObject *gstbml, GstBML *bml, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick)) {
  gboolean changed=FALSE;

  if(beats_per_minute>=0) {
    if(bml->beats_per_minute!=beats_per_minute) {
      bml->beats_per_minute=(gulong)beats_per_minute;
      g_object_notify(gstbml,"beats-per-minute");
      changed=TRUE;
    }
  }
  if(ticks_per_beat>=0) {
    if(bml->ticks_per_beat!=ticks_per_beat) {
      bml->ticks_per_beat=(gulong)ticks_per_beat;
      g_object_notify(gstbml,"ticks-per-beat");
      changed=TRUE;
    }
  }
  if(subticks_per_tick>=0) {
    if(bml->subticks_per_tick!=subticks_per_tick) {
      bml->subticks_per_tick=(gulong)subticks_per_tick;
      g_object_notify(gstbml,"subticks-per-tick");
      changed=TRUE;
    }
  }
  if(changed) {
    GST_INFO("changing tempo to %d BPM  %d TPB  %d STPT",bml->beats_per_minute,bml->ticks_per_beat,bml->subticks_per_tick);
    gstbml_calculate_buffer_frames(bml);
    // update timevalues in buzzmachine
    bml(set_master_info(bml->beats_per_minute,bml->ticks_per_beat,bml->samplerate));
    bml(init(bml->bm,0,NULL));
  }
}

//-- common class functions

/*
 * gstbml_class_base_init:
 *
 * store common class params
 */
gpointer bml(gstbml_class_base_init(GstBMLClass *klass, GType type, gint numsrcpads, gint numsinkpads)) {
  gpointer bm;
  gchar *dll_name;

  GST_INFO("initializing base: type=0x%p, voice_type=0x%p",type,voice_type);

  bm=g_hash_table_lookup(bml_descriptors_by_element_type,GINT_TO_POINTER(type));
  if(!bm) bm=g_hash_table_lookup(bml_descriptors_by_element_type,GINT_TO_POINTER(0));
  g_assert(bm);

  dll_name=g_hash_table_lookup(bml_dllpath_by_element_type,GINT_TO_POINTER(type));
  if(!dll_name) dll_name=g_hash_table_lookup(bml_dllpath_by_element_type,GINT_TO_POINTER(0));
  g_assert(dll_name);

  GST_INFO("initializing base: bm=0x%p, dll_name=%s",bm,dll_name);

  klass->bm=bm;
  klass->dll_name=dll_name;
  klass->help_uri=g_hash_table_lookup(bml_help_uri_by_descriptor,GINT_TO_POINTER(bm));
  klass->preset_path=g_hash_table_lookup(bml_preset_path_by_descriptor,GINT_TO_POINTER(bm));
  klass->voice_type=voice_type;
  klass->numsrcpads=numsrcpads;
  klass->numsinkpads=numsinkpads;

  if(!bml(get_machine_info(bm,BM_PROP_NUM_INPUT_CHANNELS,(void *)&klass->input_channels)) ||
    !bml(get_machine_info(bm,BM_PROP_NUM_OUTPUT_CHANNELS,(void *)&klass->output_channels))) {

    gint flags;

    bml(get_machine_info(bm,BM_PROP_FLAGS,(void *)&flags));
    klass->input_channels=klass->output_channels=1;
    // MIF_MONO_TO_STEREO
    if(flags&1) klass->output_channels=2;
  }

  return(bm);
}

/*
 * gstbml_class_set_details:
 *
 * Get metadata from buzz-machine and set as element details
 */
void bml(gstbml_class_set_details(GstElementClass *klass, gpointer bm, const gchar *category)) {
  GstElementDetails *details;
  gchar *str;

  /* construct the element details struct */
  details=g_new(GstElementDetails,1);
  bml(get_machine_info(bm,BM_PROP_SHORT_NAME,(void *)&str));
  details->longname=g_convert(str,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
  bml(get_machine_info(bm,BM_PROP_NAME,(void *)&str));
  details->description=g_convert(str,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
  bml(get_machine_info(bm,BM_PROP_AUTHOR,(void *)&str));
  details->author=g_convert(str,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
  details->klass = (gchar *)category;
  gst_element_class_set_details(klass, details);
  GST_DEBUG("  element_class details have been set");
}


static GType gst_bml_register_global_enum_type(GObjectClass *klass, gpointer bm, gint i, gchar *name, gint min_val, gint max_val, gint no_val) {
  GType enum_type=0;
  const gchar *desc;

  desc=bml(describe_global_value(bm, i, min_val));
  GST_INFO("check enum, description='%s', (entries=(%d-%d)=%d), no_val=%d",desc,max_val,min_val,((max_val+1)-min_val),no_val);
  if(desc && g_ascii_isalpha(desc[0])) {
    gchar *type_name;

    // build type name
    type_name=g_strdup_printf("%s%s",g_type_name(G_TYPE_FROM_CLASS(klass)),name);
    if(!(enum_type=g_type_from_name(type_name))) {
      gint j,k,total=(max_val+1)-min_val,count=0;
      GEnumValue *enums;

      for(j=0;j<total;j++) {
        desc=bml(describe_global_value(bm, i, min_val+j));
        //if((j==no_val) && !desc) desc=" ";
        if(desc && g_ascii_isalpha(desc[0])) {
          count++;
          GST_DEBUG("check enum, description[%2d]='%s'",j,desc);
        }
      }

      // DEBUG
      //if(count>50) {
      //  GST_WARNING("lots of entries (%d-%d)=%d)?",max_val,min_val,(max_val-min_val));
      //  count=50;
      //}
      // DEBUG
      
      // some plugins just have text for val=min, or val=min/max
      if(total-count<2) {
        // this we can never free :(
        enums=g_new(GEnumValue, count+2);
        // create an enum type
        for(j=k=0;j<total;j++) {
          desc=bml(describe_global_value(bm, i, min_val+j));
          //if((j==no_val) && !desc) desc=" ";
          if(desc && g_ascii_isalpha(desc[0])) {
            enums[k].value=min_val+j;
            // we have to copy these as buzzmachines can reuse the memory we get from describe()
            enums[k].value_nick=enums[k].value_name=g_strdup((gchar *)desc);
            k++;
          }
        }
        enums[k].value=no_val;
        enums[k].value_name="";
        enums[k].value_nick="";
        k++;
        // terminator
        enums[k].value=0;
        enums[k].value_name=NULL;
        enums[k].value_nick=NULL;
  
        enum_type = g_enum_register_static (type_name, enums);
        GST_INFO("register enum '%s' with %d values",type_name,count);
      }
      else {
        GST_INFO("not making enum '%s' with %d of %d values",type_name,count,total);
      }
    }
    else {
      GST_INFO("existing enum '%s'",type_name);
    }
    g_free(type_name);
  }
  return(enum_type);
}

GType bml(gstbml_register_track_enum_type(GObjectClass *klass, gpointer bm, gint i, gchar *name, gint min_val, gint max_val, gint no_val)) {
  GType enum_type=0;
  const gchar *desc;

  desc=bml(describe_track_value(bm, i, min_val));
  GST_INFO("check enum, description= '%s', (entries=(%d-%d)=%d), no_val=%d",desc,max_val,min_val,((max_val+1)-min_val),no_val);
  if(desc && g_ascii_isalpha(desc[0])) {
    gchar *type_name;
    const gchar *class_type_name;

    // build type name
    // we need to avoid creating this for GstBML and GstBMLV
    // @todo: if we have done it for GstBML, can't we look it up and return it ?
    class_type_name=g_type_name(G_TYPE_FROM_CLASS(klass));
    if(strncmp(class_type_name,"bmlv-",5)) {
      type_name=g_strdup_printf("%s%s",class_type_name,name);
    }
    else {
      type_name=g_strdup_printf("bmlv-%s%s",&class_type_name[5],name);
    }
    if(!(enum_type=g_type_from_name(type_name))) {
      gint j,k,total=(max_val+1)-min_val,count=0;
      GEnumValue *enums;

      for(j=0;j<total;j++) {
        desc=bml(describe_track_value(bm, i, min_val+j));
        if(desc && g_ascii_isalpha(desc[0])) {
          count++;
          GST_DEBUG("check enum, description[%2d]='%s'",j,desc);
        }
      }

      // some plugins just have text for val=min, or val=min/max
      if(total-count<2) {
        // this we can never free :(
        enums=g_new(GEnumValue, count+2);
        // create an enum type
        for(j=k=0;j<total;j++) {
          desc=bml(describe_track_value(bm, i, min_val+j));
          if(desc && g_ascii_isalpha(desc[0])) {
            enums[k].value=min_val+j;
            // we have to copy these as buzzmachines can reuse the memory we get from describe()
            enums[k].value_nick=enums[k].value_name=g_strdup((gchar *)desc);
            k++;
          }
        }
        enums[k].value=no_val;
        enums[k].value_name="";
        enums[k].value_nick="";
        k++;
        // terminator
        enums[k].value=0;
        enums[k].value_name=NULL;
        enums[k].value_nick=NULL;
  
        enum_type = g_enum_register_static (type_name, enums);
        GST_INFO("register enum '%s' with %d values",type_name,count);
      }
      else {
        GST_INFO("not making enum '%s' with %d of %d values",type_name,count,total);
      }
    }
    else {
      GST_INFO("existing enum '%s'",type_name);
    }
    g_free(type_name);
  }
  return(enum_type);
}

/*
 * gstbml_class_prepare_properties:
 *
 * override interface properties and register class properties
 */
void bml(gstbml_class_prepare_properties(GObjectClass *klass, GstBMLClass *bml_class)) {
  gpointer bm=bml_class->bm;
  GType enum_type = 0;
  gint i, flags, type;
  gint num,min_val,max_val,def_val,no_val;
  gchar *tmp_name,*tmp_desc;
  gchar *name,*nick,*desc;
  gint prop_id=ARG_LAST;

  // override interface properties
  g_object_class_override_property(klass, ARG_BPM, "beats-per-minute");
  g_object_class_override_property(klass, ARG_TPB, "ticks-per-beat");
  g_object_class_override_property(klass, ARG_STPT, "subticks-per-tick");

  g_object_class_install_property (klass, ARG_HOST_CALLBACKS,
    g_param_spec_pointer ("host-callbacks",
    "host-callbacks property",
    "Buzz host callback structure",
    G_PARAM_WRITABLE));
    
  if(bml(gstbml_is_polyphonic(bm))) {
    g_object_class_override_property(klass, prop_id, "children");
    prop_id++;
  }
  if(bml_class->help_uri) {
    g_object_class_override_property(klass, prop_id, "documentation-uri");
    prop_id++;
  }

  // register attributes as gobject properties
  if(bml(get_machine_info(bm,BM_PROP_NUM_ATTRIBUTES,(void *)&num))) {
    GST_INFO("  machine has %d attributes ",num);
    for(i=0;i<num;i++,prop_id++) {
      GST_DEBUG("      attribute=%02i",i);
      bml(get_attribute_info(bm,i,BM_ATTR_NAME,(void *)&tmp_name));
      bml(get_attribute_info(bm,i,BM_ATTR_MIN_VALUE,(void *)&min_val));
      bml(get_attribute_info(bm,i,BM_ATTR_MAX_VALUE,(void *)&max_val));
      bml(get_attribute_info(bm,i,BM_ATTR_DEF_VALUE,(void *)&def_val));
      gstbml_convert_names(klass, tmp_name, tmp_name, &name, &nick, &desc);
      if(gstbml_register_param(klass, prop_id, PT_ATTR, 0, name, nick, desc, 0, min_val, max_val, 0, def_val)) {
        bml_class->numattributes++;
      }
      else {
        GST_WARNING("registering attribute failed!");
      }
      g_free(name);g_free(nick);g_free(desc);
    }
  }
  GST_INFO("  %d attribute installed",bml_class->numattributes);

  // register global params as gobject properties
  if(bml(get_machine_info(bm,BM_PROP_NUM_GLOBAL_PARAMS,(void *)&num))) {
    GST_INFO("  machine has %d global params ",num);
    for(i=0;i<num;i++,prop_id++) {
      GST_DEBUG("      global_param=%02i",i);
      if(bml(get_global_parameter_info(bm,i,BM_PARA_TYPE,(void *)&type)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_NAME,(void *)&tmp_name)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_DESCRIPTION,(void *)&tmp_desc)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_FLAGS,(void *)&flags)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_MIN_VALUE,(void *)&min_val)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_MAX_VALUE,(void *)&max_val)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_NO_VALUE,(void *)&no_val)) &&
        bml(get_global_parameter_info(bm,i,BM_PARA_DEF_VALUE,(void *)&def_val))
      ) {
        gstbml_convert_names(klass, tmp_name, tmp_desc, &name, &nick, &desc);
        // create an enum on the fly
        if(type==PT_BYTE) {
          if((enum_type = gst_bml_register_global_enum_type(klass, bm, i, name, min_val, max_val, no_val))) {
            type = PT_ENUM;
          }
        }

        // don't know why buzzmachines have different defval and current val after init ...
        //def_val=bml(get_global_parameter_value(bm,i));
        //addr=bml(get_global_parameter_location(bm,i));
        if(gstbml_register_param(klass, prop_id, type, enum_type, name, nick, desc, flags, min_val, max_val, no_val, def_val)) {
          bml_class->numglobalparams++;
        }
        else {
          GST_WARNING("registering global_param failed!");
        }
        g_free(name);g_free(nick);g_free(desc);
      }
    }
  }
  GST_INFO("  %d global params installed",bml_class->numglobalparams);

  // register track params as gobject properties
  if(bml(get_machine_info(bm,BM_PROP_NUM_TRACK_PARAMS,(void *)&num))) {
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
        gstbml_convert_names(klass, tmp_name, tmp_desc, &name, &nick, &desc);
        // create an enum on the fly
        if(type==PT_BYTE) {
          if((enum_type = bml(gstbml_register_track_enum_type(klass, bm, i, name, min_val, max_val, no_val)))) {
            type = PT_ENUM;
          }
        }
        if(gstbml_register_param(klass, prop_id, type, enum_type, name, nick, desc, flags, min_val, max_val, no_val, def_val)) {
          bml_class->numtrackparams++;
        }
        else {
          GST_WARNING("registering global_param (pseudo track param) failed!");
        }
        g_free(name);g_free(nick);g_free(desc);
      }
    }
  }
  GST_INFO("  %d track params installed",bml_class->numtrackparams);
}

//-- common element functions

// not wrapper/native specific
/*
 * gst_bml_add_voice:
 *
 * create a new voice and add it to the list of voices
 */
static GstBMLV *gst_bml_add_voice(GstBML *bml,GType voice_type) {
  GstBMLV *bmlv;
  gchar *name;

  GST_DEBUG("adding a new voice to %p, current nr of voices is %d",bml->self,bml->num_voices);

  bmlv=g_object_new(voice_type,NULL);
  //bmlv->parent=bml;
  bmlv->bm=bml->bm;
  bmlv->voice=bml->num_voices;

  name=g_strdup_printf("voice%02d",bmlv->voice);
  // set name based on track number
  gst_object_set_name(GST_OBJECT(bmlv),name);
  g_free(name);
  // this refs the bmlv instance
  gst_object_set_parent(GST_OBJECT(bmlv),GST_OBJECT(bml->self));

  bml->voices=g_list_append(bml->voices,bmlv);
  bml->num_voices++;

  GST_DEBUG("added a new voice");
  return(bmlv);
}

// not wrapper/native specific
/*
 * gst_bml_del_voice:
 *
 * delete last voice and remove it from the list of voices
 */
static void gst_bml_del_voice(GstBML *bml,GType voice_type) {
  GList *node;
  GstObject *obj;

  GST_DEBUG("removing last voice to %p, current nr of voices is %d",bml->self,bml->num_voices);

  node=g_list_last(bml->voices);
  obj=node->data;
  GST_DEBUG("  free voice : %p (%d)",obj,G_OBJECT(obj)->ref_count);
  gst_object_unparent(obj);
  // no need to unref, the unparent does that
  //g_object_unref(node->data);

  bml->voices=g_list_delete_link(bml->voices,node);
  bml->num_voices--;

  GST_DEBUG("removed last voice");
}

/*
 * gst_bml_init_voices:
 *
 * initialize voice of a new instance
 */
static void gst_bml_init_voices(GstBML *bml,GstBMLClass *klass) {
  gpointer bm=bml->bm;

  GST_INFO("initializing voices: bml=%p, bml_class=%p",bml,klass);

  bml->num_voices=0;
  bml->voices=NULL;
  if(bml(gstbml_is_polyphonic(bm))) {
    gint i,min_voices;

    GST_DEBUG("instantiating default voices");

    // add voice instances
    if(bml(get_machine_info(bm,BM_PROP_MIN_TRACKS,(void *)&min_voices))) {
      GST_DEBUG("adding %d voices",min_voices);
      for(i=0;i<min_voices;i++) {
        gst_bml_add_voice(bml,klass->voice_type);
      }
    }
    else {
      GST_WARNING("failed to get min voices");
    }
  }
}

/*
 * gstbml_init:
 *
 * initialize the new instance
 */
void bml(gstbml_init(GstBML *bml,GstBMLClass *klass,GstElement *element)) {
  GST_DEBUG("init: element=%p, bml=%p, bml_class=%p",element,bml,klass);

  bml->self=element;
  bml->bm=klass->bm;

  /* @todo: shouldn't we do here: */
  bml->bm=bml(new(klass->dll_name));
  /* */
  bml(init(bml->bm,0,NULL));

  gst_bml_init_voices(bml,klass);

  // allocate the various arrays
  bml->srcpads=g_new0(GstPad *,klass->numsrcpads);
  bml->sinkpads=g_new0(GstPad *,klass->numsinkpads);

  // nonzero default needed to instantiate() some plugins
  bml->samplerate=GST_AUDIO_DEF_RATE;
  bml->beats_per_minute=120;
  bml->ticks_per_beat=4;
  bml->subticks_per_tick=1;
  gstbml_calculate_buffer_frames(bml);
  bml(set_master_info(bml->beats_per_minute,bml->ticks_per_beat,bml->samplerate));
  GST_DEBUG("activating %d voice(s)",bml->num_voices);
  //bml(set_num_tracks(bml,bml->num_voices));
}

/*
 * gstbml_init_pads
 *
 * add pads
 */
void bml(gstbml_init_pads(GstElement *element, GstBML *bml, GstPadLinkFunction *gst_bml_link)) {
  GstElementClass *klass=GST_ELEMENT_GET_CLASS(element);
  GstPad *pad;
  GList *l;
  gint sinkcount=0,srccount=0;

  for(l=klass->padtemplates;l;l=l->next) {
    pad=gst_pad_new_from_template(GST_PAD_TEMPLATE(l->data),GST_PAD_TEMPLATE_NAME_TEMPLATE(l->data));

    //gst_pad_set_link_function(pad,gst_bml_link);
    gst_element_add_pad(element,pad);

    if(GST_PAD_DIRECTION(pad)==GST_PAD_SINK) {
      bml->sinkpads[sinkcount++]=pad;
      GST_INFO("  added sinkpad");
    }
    else {
      bml->srcpads[srccount++]=pad;
      GST_INFO("  added srcpad");
    }
  }
  GST_INFO("  src_ct=%d, sink_ct=%d",srccount,sinkcount);
}

void bml(gstbml_finalize(GstBML *bml)) {
  GST_DEBUG("!!!! bml=%p",bml);

  // free list of voices
  if(bml->voices) {
    g_list_free(bml->voices);
    bml->voices=NULL;
  }
  
  g_free(bml->srcpads);
  g_free(bml->sinkpads);

  /* @todo: free here */
  bml(free(bml->bm));
  /* */
}

void bml(gstbml_set_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, const GValue *value, GParamSpec * pspec)) {
  gboolean handled=FALSE;
  gpointer bm=bml->bm;
  guint props_skip=ARG_LAST-1;

  GST_DEBUG ("prop-id %d", prop_id);

  switch(prop_id) {
    // handle properties <ARG_LAST first
    case ARG_BPM:
    case ARG_TPB:
    case ARG_STPT:
      GST_WARNING("use gst_bml_tempo_change_tempo()");
      handled=TRUE;
      break;
    case ARG_HOST_CALLBACKS:
      // supply the callbacks to bml
      GST_DEBUG("passing callbacks to bml");
      bml(set_callbacks(bm,g_value_get_pointer(value)));
      handled=TRUE;
      break;      
    default:
      if(bml(gstbml_is_polyphonic(bm))) {
        if(prop_id==(props_skip+1)) {
          gulong i;
          gulong old_num_voices=bml->num_voices;
          gulong new_num_voices = g_value_get_ulong(value);
          GST_DEBUG ("change number of voices from %d to %d", old_num_voices,new_num_voices);
          // add or free voices
          if(old_num_voices<new_num_voices) {
            for(i=old_num_voices;i<new_num_voices;i++) {
              // this increments bml->num_voices
              gst_bml_add_voice(bml,bml_class->voice_type);
            }
          }
          else {
            for(i=new_num_voices;i<old_num_voices;i++) {
              // this decrements bml->num_voices
              gst_bml_del_voice(bml,bml_class->voice_type);
            }
          }
          if(old_num_voices!=new_num_voices) {
            bml(set_num_tracks(bm,bml->num_voices));
          }
          handled=TRUE;
        }
        props_skip++;
      }
      if(bml_class->help_uri) {
        if(prop_id==(props_skip+1)) {
          GST_WARNING ("documentation-uri property is read only");
          handled=TRUE;
        }
        props_skip++;
      }
      break;
  }
  prop_id-=props_skip;

  // pass remaining props to wrapped plugin
  if(!handled) {
    gpointer addr;
    gint type;
    // DEBUG
    gchar *valstr;
    // DEBUG

    g_assert(prop_id>0);

    type=GPOINTER_TO_INT(g_param_spec_get_qdata(pspec,gst_bml_property_meta_quark_type));
    prop_id--;
    GST_LOG("id: %d, attr: %d, global: %d, voice: %d",prop_id,bml_class->numattributes,bml_class->numglobalparams,bml_class->numtrackparams);
     // is it an attribute ?
    if(prop_id<bml_class->numattributes) {
      bml(set_attribute_value(bm,prop_id,g_value_get_int(value)));
      GST_DEBUG("set attribute %d to %d", prop_id, g_value_get_int(value));
    }
    else {
      prop_id-=bml_class->numattributes;
      // is it a global param
      if(prop_id<bml_class->numglobalparams) {
        addr=bml(get_global_parameter_location(bm,prop_id));
        //bml(get_global_parameter_info(bm,prop_id,BM_PARA_TYPE,(void *)&type));
        gstbml_set_param(type,addr,value);
        valstr=g_strdup_value_contents(value);
        GST_DEBUG("set global param %d to %s (%p)", prop_id, valstr,addr);
        g_free(valstr);
      }
      else {
        prop_id-=bml_class->numglobalparams;
        // is it a voice00 param
        if(prop_id<bml_class->numtrackparams) {
          addr=bml(get_track_parameter_location(bm,0,prop_id));
          // @todo cache this info
          //bml(get_track_parameter_info(bm,prop_id,BM_PARA_TYPE,(void *)&type));
          gstbml_set_param(type,addr,value);
          valstr=g_strdup_value_contents(value);
          GST_DEBUG("set track param %d:0 to %s (%p)", prop_id, valstr,addr);
          g_free(valstr);
        }
      }
    }
  }
}

void bml(gstbml_get_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, GValue *value, GParamSpec * pspec)) {
  gboolean handled=FALSE;
  gpointer bm=bml->bm;
  guint props_skip=ARG_LAST-1;

  GST_DEBUG ("prop-id %d", prop_id);

  switch(prop_id) {
    // handle properties <ARG_LAST first
    case ARG_BPM:
      g_value_set_ulong(value, bml->beats_per_minute);
      GST_DEBUG("requested BPM = %d", bml->beats_per_minute);
      handled=TRUE;
      break;
    case ARG_TPB:
      g_value_set_ulong(value, bml->ticks_per_beat);
      GST_DEBUG("requested TPB = %d", bml->ticks_per_beat);
      handled=TRUE;
      break;
    case ARG_STPT:
      g_value_set_ulong(value, bml->subticks_per_tick);
      GST_DEBUG("requested STPB = %d", bml->subticks_per_tick);
      handled=TRUE;
      break;
    /*case ARG_HOST_CALLBACKS:
      GST_WARNING ("callbacks property is write only");
      handled=TRUE;
      break;*/
    default:
      if(bml(gstbml_is_polyphonic(bm))) {
        if(prop_id==(props_skip+1)) {
          g_value_set_ulong(value, bml->num_voices);
          GST_DEBUG("requested number of voices = %d", bml->num_voices);
          handled=TRUE;
        }
        props_skip++;
      }
      if(bml_class->help_uri) {
        if(prop_id==(props_skip+1)) {
          g_value_set_string(value,bml_class->help_uri);
          handled=TRUE;
        }
        props_skip++;
      }
      break;
  }
  prop_id-=props_skip;

  // pass remaining props to wrapped plugin
  if(!handled) {
    gpointer addr;
    gint type;
    // DEBUG
    gchar *valstr;
    // DEBUG

    g_assert(prop_id>0);

    type=GPOINTER_TO_INT(g_param_spec_get_qdata(pspec,gst_bml_property_meta_quark_type));
    prop_id--;
    GST_DEBUG("id: %d, attr: %d, global: %d, voice: %d",prop_id,bml_class->numattributes,bml_class->numglobalparams,bml_class->numtrackparams);
     // is it an attribute ?
    if(prop_id<bml_class->numattributes) {
      g_value_set_int(value,bml(get_attribute_value(bm,prop_id)));
      GST_DEBUG("got attribute as %d", g_value_get_int(value));
    }
    else {
      prop_id-=bml_class->numattributes;
      // is it a global param
      if(prop_id<bml_class->numglobalparams) {
        addr=bml(get_global_parameter_location(bm,prop_id));
        // @todo cache this info
        //bml(get_global_parameter_info(bm,prop_id,BM_PARA_TYPE,(void *)&type));
        gstbml_get_param(type,addr,value);
        valstr=g_strdup_value_contents(value);
        GST_DEBUG ("got track param as %s (%p)", valstr,addr);
          g_free(valstr);
      }
    }
  }
}

