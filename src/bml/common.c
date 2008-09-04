/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * common.c: Functions shared among all native and wrapped elements
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

extern GHashTable *bml_help_uri_by_descriptor;
extern GHashTable *bml_preset_path_by_descriptor;

//-- helper

/*
 * gstbml_get_help_uri:
 *
 * Get the uri of the user-docs for a buzzmachine.
 *
 * Returns: the uri or %NULL if machine has no docs. Do not modify.
 */
const gchar *gstbml_get_help_uri(gpointer bm) {
  return((const gchar *)g_hash_table_lookup(bml_help_uri_by_descriptor,bm));
}

/*
 * gstbml_get_preset_path:
 *
 * Get the filepath of the presets for a buzzmachine.
 *
 * Returns: the path or %NULL if machine has no presets. Do not modify.
 */
const gchar *gstbml_get_preset_path(gpointer bm) {
  return((const gchar *)g_hash_table_lookup(bml_preset_path_by_descriptor,bm));
}

//-- preset iface

gchar** gstbml_preset_get_preset_names(GstBML *bml,GstBMLClass *klass) {
  if(!klass->presets) {
    if(klass->preset_path) {
      FILE *in;

      if((in=fopen(klass->preset_path,"rb"))) {
        guint32 i,version,size,count;
        guint32 tracks,params;
        guint32 *data;
        gchar *machine_name,*preset_name,*comment;

        // read header
        if(!(fread(&version,sizeof(version),1,in))) goto eof_error;
        if(!(fread(&size,sizeof(size),1,in))) goto eof_error;

        machine_name=g_malloc0(size+1);
        if(!(fread(machine_name,size,1,in))) goto eof_error;

        // @todo: need to cut off path and '.dll'
        if(!strstr(klass->dll_name,machine_name)) {
          GST_WARNING("Preset for wrong machine? '%s' <> '%s'",klass->dll_name,machine_name);
        }

        if(!(fread(&count,sizeof(count),1,in))) goto eof_error;

        GST_INFO("reading %ld presets for machine '%s' (version %ld, dllname '%s')",
          count,machine_name,version,klass->dll_name);

        // create data hash if its not there
        if(!klass->preset_data) {
          klass->preset_data=g_hash_table_new(g_str_hash,g_str_equal);
        }
        if(!klass->preset_comments) {
          klass->preset_comments=g_hash_table_new(g_str_hash,g_str_equal);
        }

        // read presets
        for(i=0;i<count;i++) {
          if(!(fread(&size,sizeof(size),1,in))) goto eof_error;

          preset_name=g_malloc0(size+1);
          if(!(fread(preset_name,size,1,in))) goto eof_error;
          GST_INFO("  reading preset %d: %p '%s'",i,preset_name,preset_name);
          if(!(fread(&tracks,sizeof(tracks),1,in))) goto eof_error;
          if(!(fread(&params,sizeof(params),1,in))) goto eof_error;
          // read preset data
          GST_INFO("  data size %ld",(4*(2+params)));
          data=g_malloc(4*(2+params));
          data[0]=tracks;
          data[1]=params;
          if(!(fread(&data[2],4*params,1,in))) goto eof_error;
          g_hash_table_insert(klass->preset_data,(gpointer)preset_name,(gpointer)data);

          if(!(fread(&size,sizeof(size),1,in))) goto eof_error;

          if(size) {
            comment=g_malloc0(size+1);
            if(!(fread(comment,size,1,in))) goto eof_error;
            g_hash_table_insert(klass->preset_comments,(gpointer)preset_name,(gpointer)comment);
          }
          else comment=NULL;

          GST_INFO("    %ld tracks, %d params, comment '%s'",tracks,params,comment);

          klass->presets=g_list_insert_sorted(klass->presets,(gpointer)preset_name,(GCompareFunc)strcmp);
        }
        g_free(machine_name);

eof_error:
        fclose(in);
      }
      else {
        GST_INFO("can't open preset file: '%s'",klass->preset_path);
      }
    }
    else {
      GST_INFO("no preset path for machine");
    }
  }
  else {
    GST_INFO("return cached preset list");
  }
  if (klass->presets) {
    gchar **preset_names = g_new (gchar*, g_list_length (klass->presets) + 1);
    GList *node;
    guint i=0;
    
    for (node = klass->presets; node; node = g_list_next (node)) {
      preset_names[i++] = g_strdup(node->data);
    }
    preset_names[i]=NULL;
    return (preset_names);
  }
  return (NULL);
}

gboolean gstbml_preset_load_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name) {
  GList *node;
  guint32 *data;

  if(!klass->presets) {
    if(!(gstbml_preset_get_preset_names(bml,klass))) {
      GST_INFO("no presets for machine");
      return(FALSE);
    }
  }

  if((node=g_list_find_custom(klass->presets,name,(GCompareFunc)strcmp))) {
    // get preset data record
    if((data=g_hash_table_lookup(klass->preset_data,node->data))) {
      guint32 i,tracks,params;
      //GType param_type,base_type;
      GObjectClass *voice_class=G_OBJECT_CLASS(g_type_class_ref(klass->voice_type));
      GParamSpec **properties,*property;
      guint number_of_properties;
      guint flags;

      tracks=*data++;
      params=*data++;

      GST_INFO("about to load preset '%s' with %d tracks and %d total params",name,tracks,params);
      GST_INFO("machine has %d global and %d voice params",klass->numglobalparams,klass->numtrackparams);

      // set global parameters
      if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self)),&number_of_properties))) {
        for(i=0;i<number_of_properties;i++) {
          property=properties[i];
          flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

          // skip non-controlable, trigger params & voice params
          if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
          else if(!(flags&GST_PROPERTY_META_STATE)) continue;
          else if(voice_class && g_object_class_find_property(voice_class,property->name)) continue;
          // set parameters
          g_object_set(self,property->name,*data++,NULL);
        }
      }

      // set track times voice parameters
      if(voice_class && bml->num_voices) {
        if((properties=g_object_class_list_properties(voice_class,&number_of_properties))) {
          GstBMLV *voice;
          GList *node;
          guint32 j;

          for(j=0,node=bml->voices;(j<tracks && node);j++,node=g_list_next(node)) {
            voice=node->data;
            for(i=0;i<number_of_properties;i++) {
              property=properties[i];
              flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

              // skip trigger params
              if(!(flags&GST_PROPERTY_META_STATE)) continue;

              // set parameters
              g_object_set(voice,property->name,*data++,NULL);
            }
          }
        }
      }

      if(voice_class) g_type_class_unref(voice_class);
    }
    else {
      GST_WARNING("no preset data for '%s'",name);
    }
  }
  else {
    GST_WARNING("no preset for '%s'",name);
  }
  return(FALSE);
}

static gboolean gstbml_preset_save_presets_file(GstBMLClass *klass) {
  if(klass->preset_path) {
    FILE *out;
    gchar *bak_file_name;
    gboolean backup=TRUE;

    // create backup if possible
    bak_file_name=g_strdup_printf("%s.bak",klass->preset_path);
    if(g_file_test(bak_file_name, G_FILE_TEST_EXISTS)) {
      if(g_unlink(bak_file_name)) {
        backup=FALSE;
        GST_INFO("cannot remove old backup file : %s",bak_file_name);
      }
    }
    if(backup) {
      if(g_rename(klass->preset_path,bak_file_name)) {
        GST_INFO("cannot backup file : %s -> %s",klass->preset_path,bak_file_name);
      }
    }
    g_free(bak_file_name);

    GST_INFO("about to save presets '%s'",klass->preset_path);

    if((out=fopen(klass->preset_path,"wb"))) {
      guint32 version,size,count;
      gchar *machine_name,*ext,*preset_name,*comment;
      guint32 *data;
      GList *node;

      // write header
      version=1;
      if(!(fwrite(&version,sizeof(version),1,out))) goto eof_error;

      machine_name=strrchr(klass->dll_name,'/');
      ext=strstr(machine_name,".dll");
      size=(gulong)ext-(gulong)machine_name;
      if(!(fwrite(&size,sizeof(size),1,out))) goto eof_error;
      if(!(fwrite(machine_name,size,1,out))) goto eof_error;

      count=g_list_length(klass->presets);
      if(!(fwrite(&count,sizeof(count),1,out))) goto eof_error;

      // write presets
      for(node=klass->presets;node;node=g_list_next(node)) {
        preset_name=node->data;
        size=strlen(preset_name);
        if(!(fwrite(&size,sizeof(size),1,out))) goto eof_error;
        if(!(fwrite(preset_name,size,1,out))) goto eof_error;
        // write preset data (data[1]=params)
        data=g_hash_table_lookup(klass->preset_data,(gpointer)preset_name);
        GST_INFO("  name %p,'%s'",preset_name,preset_name);
        //GST_INFO("  data size [%s]: %ld",preset_name,(4*(2+data[1])));
        if(!(fwrite(data,(4*(2+data[1])),1,out))) goto eof_error;

        // handle comments
        comment=g_hash_table_lookup(klass->preset_comments,(gpointer)preset_name);
        size=comment?strlen(comment):0;
        if(!(fwrite(&size,sizeof(size),1,out))) goto eof_error;
        if(comment) {
          if(!(fwrite(comment,size,1,out))) goto eof_error;
        }
      }
eof_error:
      fclose(out);
      return(TRUE);
    }
  }
  return(FALSE);
}

gboolean gstbml_preset_save_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name) {
  GObjectClass *voice_class=G_OBJECT_CLASS(g_type_class_ref(klass->voice_type));
  GParamSpec **properties,*property;
  guint number_of_properties;
  guint flags;
  guint32 *data,*ptr;
  guint32 i,params,numglobalparams=0,numtrackparams=0;

  // count global preset params
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self)),&number_of_properties))) {
    for(i=0;i<number_of_properties;i++) {
      property=properties[i];
      flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

      // skip non-controlable, trigger params & voice params
      if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
      else if(!(flags&GST_PROPERTY_META_STATE)) continue;
      else if(voice_class && g_object_class_find_property(voice_class,property->name)) continue;
      numglobalparams++;
    }
  }
  // count voice preset params
  if(voice_class && bml->num_voices) {
    if((properties=g_object_class_list_properties(voice_class,&number_of_properties))) {
      for(i=0;i<number_of_properties;i++) {
        property=properties[i];
        flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

        // skip trigger params
        if(!(flags&GST_PROPERTY_META_STATE)) continue;
        numtrackparams++;
      }
    }
  }

  // create new preset
  params=numglobalparams+bml->num_voices*numtrackparams;
  GST_INFO("  data size %ld",(4*(2+params)));
  data=g_malloc(4*(2+params));
  data[0]=bml->num_voices;
  data[1]=params;
  ptr=&data[2];

  GST_INFO("about to add new preset '%s' with %d tracks and %d total params",name,bml->num_voices,params);

  // take copies of current gobject properties from self
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self)),&number_of_properties))) {
    for(i=0;i<number_of_properties;i++) {
      property=properties[i];
      flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

      // skip non-controlable, trigger params & voice params
      if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
      else if(!(flags&GST_PROPERTY_META_STATE)) continue;
      else if(voice_class && g_object_class_find_property(voice_class,property->name)) continue;
      // get parameters
      g_object_get(self,property->name,ptr++,NULL);
    }
  }
  if(voice_class && bml->num_voices) {
    if((properties=g_object_class_list_properties(voice_class,&number_of_properties))) {
      GstBMLV *voice;
      GList *node;
      guint32 j;

      for(j=0,node=bml->voices;(j<bml->num_voices && node);j++,node=g_list_next(node)) {
        voice=node->data;
        for(i=0;i<number_of_properties;i++) {
          property=properties[i];
          flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));

          // skip trigger params
          if(!(flags&GST_PROPERTY_META_STATE)) continue;

          // get parameters
          g_object_get(voice,property->name,ptr++,NULL);
        }
      }
    }
  }

  // add new entry
  g_hash_table_insert(klass->preset_data,(gpointer)name,(gpointer)data);
  klass->presets=g_list_insert_sorted(klass->presets,(gpointer)name,(GCompareFunc)strcmp);

  if(voice_class) g_type_class_unref(voice_class);

  return(gstbml_preset_save_presets_file(klass));
}

gboolean gstbml_preset_rename_preset(GstBMLClass *klass, const gchar *old_name, const gchar *new_name) {
  GList *node;

  if((node=g_list_find_custom(klass->presets,old_name,(GCompareFunc)strcmp))) {
    gpointer data;

    // move preset data record
    if((data=g_hash_table_lookup(klass->preset_data,node->data))) {
      g_hash_table_remove(klass->preset_data,node->data);
      g_hash_table_insert(klass->preset_data,(gpointer)new_name,(gpointer)data);
    }
    // move preset comment
    if((data=g_hash_table_lookup(klass->preset_comments,node->data))) {
      g_hash_table_remove(klass->preset_comments,node->data);
      g_hash_table_insert(klass->preset_comments,(gpointer)new_name,(gpointer)data);
    }

    // readd under new name
    klass->presets=g_list_delete_link(klass->presets,node);
    klass->presets=g_list_insert_sorted(klass->presets,(gpointer)new_name,(GCompareFunc)strcmp);
    GST_INFO("preset moved 's' -> '%s'",old_name,new_name);
    return(gstbml_preset_save_presets_file(klass));
  }
  return(FALSE);
}

gboolean gstbml_preset_delete_preset(GstBMLClass *klass, const gchar *name) {
  GList *node;

  if((node=g_list_find_custom(klass->presets,name,(GCompareFunc)strcmp))) {
    gpointer data;

    // remove preset data record
    if((data=g_hash_table_lookup(klass->preset_data,node->data))) {
      g_hash_table_remove(klass->preset_data,node->data);
      g_free(data);
    }
    // remove preset comment
    if((data=g_hash_table_lookup(klass->preset_comments,node->data))) {
      g_hash_table_remove(klass->preset_comments,node->data);
      g_free(data);
    }

    // remove the found one
    klass->presets=g_list_delete_link(klass->presets,node);
    GST_INFO("preset '%s' removed",name);
    g_free((gpointer)name);

    return(gstbml_preset_save_presets_file(klass));
  }
  return(FALSE);
}

gboolean gstbml_preset_set_meta(GstBMLClass *klass, const gchar *name, const gchar *tag, const gchar *value) {
  gboolean res=FALSE;

  if(!strncmp(tag,"comment\0",9)) {
    GList *node;

    if((node=g_list_find_custom(klass->presets,name,(GCompareFunc)strcmp))) {
      gchar *old_value;
      gboolean changed=FALSE;

      if((old_value=g_hash_table_lookup(klass->preset_comments,node->data))) {
        g_free(old_value);
        changed=TRUE;
      }
      if(value) {
        g_hash_table_insert(klass->preset_comments,node->data,g_strdup(value));
        changed=TRUE;
      }
      if(changed) {
        res=gstbml_preset_save_presets_file(klass);
      }
    }
  }
  return(res);
}

gboolean gstbml_preset_get_meta(GstBMLClass *klass, const gchar *name, const gchar *tag, gchar **value) {
  gboolean res=FALSE;

  if(!strncmp(tag,"comment\0",9)) {
    GList *node;

    if((node=g_list_find_custom(klass->presets,name,(GCompareFunc)strcmp))) {
      gchar *new_value;
      if((new_value=g_hash_table_lookup(klass->preset_comments,node->data))) {
        *value=g_strdup(new_value);
        res=TRUE;
      }
    }
  }
  if(!res) *value=NULL;
  return(res);
}

void gstbml_preset_finalize(GstBMLClass *klass) {
  if(klass->presets) {
    GList *node;
    gchar *name;
    gpointer data;

    // free preset_data && preset_comments
    for(node=klass->presets;node;node=g_list_next(node)) {
      name=node->data;
      // remove preset data record
      if((data=g_hash_table_lookup(klass->preset_data,name))) {
        g_hash_table_remove(klass->preset_data,name);
        g_free(data);
      }
      // remove preset comment
      if((data=g_hash_table_lookup(klass->preset_comments,name))) {
        g_hash_table_remove(klass->preset_comments,name);
        g_free(data);
      }
      g_free((gpointer)name);
    }
    g_hash_table_destroy(klass->preset_data);
    klass->preset_data=NULL;
    g_hash_table_destroy(klass->preset_comments);
    klass->preset_comments=NULL;
    g_list_free(klass->presets);
    klass->presets=NULL;
  }
}

//-- common class functions

/*
 * gstbml_convert_names:
 *
 * Convert charset encoding and make property-names unique.
 */
void gstbml_convert_names(GObjectClass *klass, gchar *tmp_name, gchar *tmp_desc, gchar **_name, gchar **nick, gchar **desc) {
  gchar *name,*ptr1,*ptr2;

  GST_DEBUG("        tmp_name='%s'",tmp_name);
  name=g_convert(tmp_name,-1,"ASCII","WINDOWS-1252",NULL,NULL,NULL);
  if(nick) {
    *nick=g_convert(tmp_name,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
  }
  if(desc && tmp_desc) {
    *desc=g_convert(tmp_desc,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
  }
  g_strcanon(name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '-');

  // remove leading '-'
  ptr1=ptr2=name;
  while(*ptr2=='-') ptr2++;
  // remove double '-'
  while(*ptr2) {
    if(*ptr2=='-') {
      while(ptr2[1]=='-') ptr2++;
    }
    if(ptr1!=ptr2) *ptr1=*ptr2;
    ptr1++;
    ptr2++;
  }
  if(ptr1!=ptr2) *ptr1='\0';
  // remove trailing '-'
  ptr1--;
  while(*ptr1=='-') *ptr1++='\0';

  // name must begin with a char
  if(!g_ascii_isalpha(name[0])) {
    gchar *old_name=name;
    name=g_strconcat("Par_",old_name,NULL);
    g_free(old_name);
  }

  // check for already existing property names
  if(g_object_class_find_property(klass, name)) {
    gchar *old_name=name;
    gchar postfix[5];
    gint i=0;

    // make name uniqe
    name=NULL;
    do {
      if(name) g_free(name);
      snprintf(postfix,5,"_%03d",i++);
      name=g_strconcat(old_name,postfix,NULL);
    } while(g_object_class_find_property(klass, name));
     g_free(old_name);
  }
  *_name=name;
}

/*
 * gstbml_register_param:
 *
 * normalize data and register property
 */
gboolean gstbml_register_param(GObjectClass *klass,gint prop_id, gint type, GType enum_type, gchar *name, gchar *nick, gchar *desc, gint flags, gint min_val, gint max_val, gint no_val, gint def_val) {
  GParamSpec *paramspec = NULL;
  gboolean res=FALSE;
  gint saved_min_val=min_val,saved_max_val=max_val,saved_def_val=def_val;

  GST_DEBUG("        name='%s', nick='%s', type=%d, flags=0x%x : val [%d ... d=%d/n=%d ... %d]",name,nick,type,flags, min_val,def_val,no_val,max_val);
  //*def = defval;
  if(def_val<min_val) {
    GST_WARNING("par=%d:%s, def_val < min_val",type,name);
    saved_min_val=min_val;
    min_val=def_val;
  }
  if(def_val>max_val) {
    GST_WARNING("par=%d:%s, def_val > max_val",type,name);
    saved_max_val=max_val;
    max_val=def_val;
  }
  if(!(flags&GST_PROPERTY_META_STATE)) {
    // only trigger params need no_val handling
    if(no_val<min_val) {
      GST_WARNING("par=%d:%s, no_val < min_val",type,name);
      min_val=no_val;
    }
    if(no_val>max_val) {
      GST_WARNING("par=%d:%s, no_val > max_val",type,name);
      max_val=no_val;
    }
    def_val=no_val;
  }
  GST_DEBUG("        val [%d ... d=%d ... %d]",min_val,def_val,max_val);

  switch(type) {
    case PT_NOTE:
      paramspec=g_param_spec_string(name, nick, desc,
          NULL, G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      /* @todo: what about using an enum type (define in gst-buzztard) here to
       * be able to detect this type in UIs
       */
      break;
    case PT_SWITCH:
      if(min_val==-1) min_val=0;
      //if(!(flags&GST_PROPERTY_META_STATE)) {
        /* @todo use better type for triggers
         * this is how its define for buzz
         * [ 0 ... n=255 ... 1]
         *
         * some machines have random stuff in here :(
         * [-1 ... d=  0/n=255 ... -1] -> [-1 ... d=255 ... 255] [-1 ...   0]
         * [ 1 ... d=255/n=255 ...  1] -> [ 1 ... d=255 ... 255] [ 1 ... 255]
         * [ 1 ... d=  0/n=255 ...  1] -> [ 0 ... d=255 ... 255]
         *
         * - we need to differenciate between PT_SWITCH as param or a trigger
         *   as trigger ones have a no-value
         * - using an uint with 0,1,255 is quite bad
         * - what about an enum OFF,ON,NO_VALUE?
         */
        /*
        paramspec=g_param_spec_uint(name, nick, desc,
          min_val,max_val,def_val,
          G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
        */
        type=PT_ENUM;
        paramspec=g_param_spec_enum(name, nick, desc,
          BT_TYPE_TRIGGER_SWITCH, BT_TRIGGER_SWITCH_EMPTY,
          G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      /*}
      else {
        // non-triggers use this
        //min_val=0;max_val=1;
        paramspec=g_param_spec_boolean(name, nick, desc,
          def_val,
          G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      }*/
      break;
    case PT_BYTE:
      // @todo gstreamer has no support for CHAR/UCHAR
      paramspec=g_param_spec_uint(name, nick, desc,
        min_val,max_val,def_val,
        G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      break;
    case PT_WORD:
      paramspec=g_param_spec_uint(name, nick, desc,
        min_val,max_val,def_val,
        G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      break;
    case PT_ENUM:
      paramspec=g_param_spec_enum(name, nick, desc,
        enum_type, def_val,
        G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE);
      break;
    case PT_ATTR: // Attribute
      paramspec=g_param_spec_int(name, nick, desc,
        min_val,max_val,def_val,
        G_PARAM_READWRITE);
      break;
    default:
      GST_WARNING("invalid type=%d",type);
      break;
  }
  if(paramspec) {
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark,GINT_TO_POINTER(TRUE));
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark_min_val, GINT_TO_POINTER(saved_min_val));
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark_max_val, GINT_TO_POINTER(saved_max_val));
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark_def_val, GINT_TO_POINTER(saved_def_val));
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark_no_val,  GINT_TO_POINTER(no_val));
    g_param_spec_set_qdata(paramspec,gst_property_meta_quark_flags,   GINT_TO_POINTER(flags));
    g_param_spec_set_qdata(paramspec,gst_bml_property_meta_quark_type,GINT_TO_POINTER(type));
    g_object_class_install_property(klass,prop_id,paramspec);
    GST_DEBUG("registered paramspec=%p",paramspec);
    res=TRUE;
  }
  return(res);
}

//-- common element functions

void gstbml_set_param(gint type,gpointer addr,const GValue *value) {
  switch(type) {
    case PT_NOTE: {
      const gchar *str=g_value_get_string(value);
      *(guint8 *)addr=(str && *str) ? gst_note_2_frequency_note_string_2_number (str) : 0;
      break;
    }
    case PT_SWITCH:
      *(guint8 *)addr=(guint8)g_value_get_boolean(value);
      //*(guint8 *)addr=(guint8)g_value_get_uint(value);
      break;
    case PT_BYTE:
      *(guint8 *)addr=(guint8)g_value_get_uint(value);
      break;
    case PT_WORD:
      *(guint16 *)addr=(guint16)g_value_get_uint(value);
      break;
    case PT_ENUM:
      *(guint8 *)addr=(guint8)g_value_get_enum(value);
      break;
    default:
      GST_WARNING("unhandled type : %d",type);
  }
}

void gstbml_get_param(gint type,gpointer addr,GValue *value) {
  switch(type) {
    case PT_NOTE:
      g_value_set_string(value, gst_note_2_frequency_note_number_2_string ((guint)(*(guint8 *)addr)));
      break;
    case PT_SWITCH:
      g_value_set_boolean(value,(guint)(*(guint8 *)addr));
      //g_value_set_uint(value,(guint)(*(guint8 *)addr));
      break;
    case PT_BYTE:
      g_value_set_uint(value,(guint)(*(guint8 *)addr));
      break;
    case PT_WORD:
      g_value_set_uint(value,(guint)(*(guint16 *)addr));
      break;
    case PT_ENUM:
      g_value_set_enum(value,(guint)(*(guint8 *)addr));
      break;
    default:
      GST_WARNING("unhandled type : %d",type);
  }
}

/*
 * gstbml_calculate_buffer_frames:
 *
 * update the buffersize for calculation (in samples)
 * buffer_frames = samples_per_minute/ticks_per_minute
 */
void gstbml_calculate_buffer_frames(GstBML *bml) {
  bml->buffer_frames=((bml->samplerate*60)/(bml->beats_per_minute*bml->ticks_per_beat));
  bml->ticktime=((GST_SECOND*60)/(GstClockTime)(bml->beats_per_minute*bml->ticks_per_beat));
}

/*
 * gstbml_sync_values:
 *
 * updates the global and voice params
 */
void gstbml_sync_values(GstBML *bml) {
  GList *node;
  //gboolean res;

  GST_DEBUG_OBJECT(bml->self,"  sync_values(%p), voices=%d,%p",bml->self,bml->num_voices,bml->voices);

  /*res=*/gst_object_sync_values(G_OBJECT(bml->self),bml->running_time);
  //GST_WARNING("glob sync %d",res);
  for(node=bml->voices;node;node=g_list_next(node)) {
    /*res=*/gst_object_sync_values(G_OBJECT(node->data),bml->running_time);
    //GST_WARNING("voice sync %d",res);
  }
}

void gstbml_dispose(GstBML *bml) {
  GList* node;

  if(bml->dispose_has_run) return;
  bml->dispose_has_run = TRUE;

  GST_DEBUG("!!!! bml=%p",bml);

  // unref list of voices
  if(bml->voices) {
    for(node=bml->voices;node;node=g_list_next(node)) {
      GstObject *obj=node->data;
      GST_DEBUG("  free voice : %p (%d)",obj,G_OBJECT(obj)->ref_count);
      gst_object_unparent(obj);
      // no need to unref, the unparent does that
      //g_object_unref(node->data);
      node->data=NULL;
    }
  }
}
