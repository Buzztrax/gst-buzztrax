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

// see http://bugzilla.gnome.org/show_bug.cgi?id=570233
//#define BUILD_STRUCTURE 1
#ifdef BUILD_STRUCTURE
extern GstStructure *bml_meta_all;
#endif

extern GstPlugin *bml_plugin;
extern GHashTable *bml_descriptors_by_element_type;
extern GHashTable *bml_descriptors_by_voice_type;
extern GHashTable *bml_voice_type_by_element_type;
extern GHashTable *bml_help_uri_by_descriptor;
extern GHashTable *bml_preset_path_by_descriptor;

static void remove_double_def_chars(gchar *name) {
  gchar *ptr1,*ptr2;

  ptr1=ptr2=name;
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
}

gboolean bml(describe_plugin(gchar *pathname, gpointer bm)) {
  gchar *dllname;
  gint type,flags;
  gboolean res=FALSE;

  GST_INFO("describing %p : %s",bm, pathname);
  
  // use dllname to register
  // 'M3.dll' and 'M3 Pro.dll' both use the name M3 :(
  // BM_PROP_DLL_NAME instead of BM_PROP_SHORT_NAME

  g_assert(bm);
  if(bml(get_machine_info(bm,BM_PROP_DLL_NAME,(void *)&dllname)) &&
    bml(get_machine_info(bm,BM_PROP_TYPE,(void *)&type)) &&
    bml(get_machine_info(bm,BM_PROP_FLAGS,(void *)&flags))
  ) {
    gchar *name,*filename,*ext;
    gchar *element_type_name,*voice_type_name=NULL;
    gchar *help_filename,*preset_filename;
    gchar *data_pathname=NULL;
    gboolean type_names_ok=TRUE;
    GType voice_type=G_TYPE_INVALID;

    if((filename=strrchr(dllname,'/'))) {
      filename++;
    }
    else {
      filename=dllname;
    }
    GST_INFO("  dll-name: '%s', flags are: %x",filename,flags);
    if(flags&0xFE) {
      GST_WARNING("  machine is not yet fully supported");
    }

    // get basename
    ext=g_strrstr(filename,".");
    *ext='\0';  // temporarily terminate
    name=g_convert_with_fallback(filename,-1,"ASCII","WINDOWS-1252","-",NULL,NULL,NULL);
    g_strstrip(name);
    *ext='.';   // restore
    GST_INFO("  name is '%s'",name);

    // construct the type name
    element_type_name = g_strdup_printf("bml-%s",name);
    if(bml(gstbml_is_polyphonic(bm))) {
      voice_type_name = g_strdup_printf("bmlv-%s",name);
    }
    g_free(name);
    // if it's already registered, skip (mean e.g. native element has been registered)
    g_strcanon(element_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+", '-');
    remove_double_def_chars(element_type_name);
    if(g_type_from_name(element_type_name)) {
      GST_WARNING("already registered type : \"%s\"", element_type_name);
      g_free(element_type_name);
      element_type_name=NULL;
        type_names_ok=FALSE;
    }
    if(voice_type_name) {
      g_strcanon(voice_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+", '-');
      remove_double_def_chars(voice_type_name);
      if(g_type_from_name(voice_type_name)) {
        GST_WARNING("already registered type : \"%s\"", voice_type_name);
        g_free(voice_type_name);
        voice_type_name=NULL;
        type_names_ok=FALSE;
      }
    }
    if(!type_names_ok) {
      g_free(element_type_name);
      g_free(voice_type_name);
      return(TRUE);
    }

    // use right path for emulated and native machines
#ifdef BML_WRAPPED
    data_pathname=g_strdup(pathname);
#else
    {
      gchar *pos;
      // replace lib in path-name wth share (should we use DATADIR/???)
      pos=strstr(pathname,"/lib/");
      if(pos) {
        *pos='\0';
        data_pathname=g_strdup_printf("%s/share/%s",pathname,&pos[5]);
        *pos='/';
      }
      else {
        GST_WARNING("failed to map plugin lib path '%s' to a datadir",pathname);
        data_pathname=g_strdup(pathname);
      }
    }
#endif
    // data files have same basename as plugin, but different extension
    ext=strrchr(data_pathname,'.');
    *ext='\0';
    // try various help uris
    help_filename = g_strdup_printf("%s.html",data_pathname);
    if(!g_file_test(help_filename, G_FILE_TEST_EXISTS)) {
      GST_INFO("no user docs at '%s'",help_filename);
      g_free(help_filename);
      help_filename = g_strdup_printf("%s.htm",data_pathname);
      if(!g_file_test(help_filename, G_FILE_TEST_EXISTS)) {
        GST_INFO("no user docs at '%s'",help_filename);
        g_free(help_filename);
        help_filename = g_strdup_printf("%s.txt",data_pathname);
        if(!g_file_test(help_filename, G_FILE_TEST_EXISTS)) {
          GST_INFO("no user docs at '%s'",help_filename);
          g_free(help_filename);
          help_filename=NULL;
        }
      }
    }
    // generate preset name
    // we need a fallback to g_get_user_config_dir()/??? in any case
    preset_filename = g_strdup_printf("%s.prs",data_pathname);
    g_free(data_pathname);

    // temporarily storing it for base-init
    g_hash_table_insert(bml_descriptors_by_element_type,GINT_TO_POINTER(0),(gpointer)bm);
    g_hash_table_insert(bml_descriptors_by_voice_type,GINT_TO_POINTER(0),(gpointer)bm);

    // store help uri
    if(help_filename) {
      gchar *help_uri;
      help_uri=g_strdup_printf("file://%s",help_filename);
      g_hash_table_insert(bml_help_uri_by_descriptor,(gpointer)bm,(gpointer)help_uri);
      GST_INFO("machine %p has user docs at '%s'",bm,help_uri);
      g_free(help_filename);
    }
    // store preset path
    if(preset_filename) {
      g_hash_table_insert(bml_preset_path_by_descriptor,(gpointer)bm,(gpointer)preset_filename);
      GST_INFO("machine %p preset path '%s'",bm,preset_filename);
    }

#ifdef BUILD_STRUCTURE
    {
#ifdef BML_WRAPPED
      GstStructure *bml_meta=gst_structure_empty_new("bmlw");
#else
      GstStructure *bml_meta=gst_structure_empty_new("bmln");
#endif
      GValue value={0,};
    
      gst_structure_set(bml_meta,
        "plugin-filename",G_TYPE_STRING,pathname,
        "machine-type",G_TYPE_INT,type,
        "element-type-name",G_TYPE_STRING,element_type_name,
        NULL);
      if(voice_type_name) {
        gst_structure_set(bml_meta,"voice-type-name",G_TYPE_STRING,voice_type_name,NULL);
      }
      if(help_filename) {
        gst_structure_set(bml_meta,"help-filename",G_TYPE_STRING,help_filename,NULL);
      }
      if(preset_filename) {
        gst_structure_set(bml_meta,"preset-filename",G_TYPE_STRING,preset_filename,NULL);
      }
    
      g_value_init(&value, GST_TYPE_STRUCTURE);
      g_value_set_boxed(&value,bml_meta);
      gst_structure_set_value(bml_meta_all,element_type_name,&value);
      g_value_unset(&value);
    }
#endif
#ifdef BUILD_STRUCTURE
 res=TRUE;
#else
    if(voice_type_name) {
      // create the voice type now
      voice_type = bml(v_get_type(voice_type_name));
      GST_INFO("  voice type \"%s\" is 0x%lu", voice_type_name,(gulong)voice_type);
      g_hash_table_insert(bml_descriptors_by_voice_type,GINT_TO_POINTER(voice_type),(gpointer)bm);
      g_free(voice_type_name);
    }
    if(element_type_name) {
      GType element_type=0L;
      // create the element type now
      switch(type) {
        case MT_MASTER: // (Sink)
          //element_type = bml(sink_get_type(element_type_name,bm));
          GST_WARNING("  unimplemented plugin type %d for '%s'",type,element_type_name);
          break;
        case MT_GENERATOR: // (Source)
          element_type = bml(src_get_type(element_type_name,(voice_type!=0),(help_filename!=NULL)));
          break;
        case MT_EFFECT: // (Processor)
          // base transform only supports elements with one source and one sink pad
          element_type = bml(transform_get_type(element_type_name,(voice_type!=0),(help_filename!=NULL)));
          break;
        default:
          GST_WARNING("  invalid plugin type %d for '%s'",type,element_type_name);
      }
      if(element_type) {
        if(voice_type) {
          g_hash_table_insert(bml_voice_type_by_element_type,GINT_TO_POINTER(element_type),(gpointer)voice_type);
        }
        g_hash_table_insert(bml_descriptors_by_element_type,GINT_TO_POINTER(element_type),(gpointer)bm);
        if(!gst_element_register(bml_plugin, element_type_name, GST_RANK_NONE, element_type)) {
          GST_ERROR("error registering new type : \"%s\"", element_type_name);
        }
        else {
          GST_INFO("succefully registered new plugin : \"%s\"", element_type_name);
          res=TRUE;
        }
      }
      g_free(element_type_name);
    }
    g_hash_table_remove(bml_descriptors_by_voice_type, GINT_TO_POINTER(0));
    g_hash_table_remove(bml_descriptors_by_element_type, GINT_TO_POINTER(0));
#endif
  }
  return(res);
}

