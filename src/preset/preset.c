/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * preset.c: helper interface for element presets
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
 * SECTION:gstpreset
 * @short_description: helper interface for element presets
 *
 * This interface offers methods to query and manipulate parameter preset sets. 
 * A preset is a bunch of property settings, together with meta data and a name.
 * The name of a preset serves as key for subsequent method calls to manipulate
 * single presets.
 * All instances of one type will share the list of presets. The list is created
 * on demand, if presets are not used, the list is not created.
 *
 */
/* @TODO:
 * - we need locks to avoid two instances manipulating the preset list -> flock
 * - need to add support for GstChildProxy
 * - how can we support both Preferences and Presets, a flag for _get_preset_names ?
 * - should there be a 'preset-list' property to get the preset list
 *   (and to connect a notify:: to to listen for changes)
 *
 * http://www.buzztard.org/index.php/Preset_handling_interface
 */

#include "preset.h"
#include "propertymeta/propertymeta.h"

#define GST_CAT_DEFAULT preset_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GQuark preset_list_quark=0;
static GQuark preset_path_quark=0;
static GQuark preset_data_quark=0;
static GQuark preset_meta_quark=0;
static GQuark instance_list_quark=0;

/* default implementation */

#define LINE_LEN 200

static gboolean
preset_get_storage(GstPreset *self,GList **presets,GHashTable **preset_meta,GHashTable **preset_data)
{
  gboolean res=FALSE;
  GType type = G_TYPE_FROM_INSTANCE (self);
  
  g_assert(presets);
  
  if((*presets = g_type_get_qdata (type, preset_list_quark))) {
	GST_DEBUG("have presets");
	res=TRUE;
  }
  if(preset_meta)  {
	if(!(*preset_meta=g_type_get_qdata (type, preset_meta_quark))) {
	  *preset_meta=g_hash_table_new(g_str_hash,g_str_equal);
	  g_type_set_qdata (type, preset_meta_quark, (gpointer) *preset_meta);
	  GST_DEBUG("new meta hash");
	}
  }
  if(preset_data) {
	if(!(*preset_data=g_type_get_qdata (type, preset_data_quark))) {
	  *preset_data=g_hash_table_new(g_str_hash,g_str_equal);
	  g_type_set_qdata (type, preset_data_quark, (gpointer) *preset_data);
	  GST_DEBUG("new data hash");
	}
  }
  GST_INFO("%ld:%s: presets: %p, %p, %p",type,G_OBJECT_TYPE_NAME(self),*presets,(preset_meta?*preset_meta:0),(preset_data?*preset_data:0));
  return(res);
}

static const gchar*
preset_get_path(GstPreset *self)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  gchar *preset_path;
  
  preset_path = (gchar *) g_type_get_qdata (type, preset_path_quark);
  if(!preset_path) {
    const gchar *element_name, *plugin_name, *file_name;
    gchar *preset_dir;
    GstElementFactory *factory;
    GstPlugin *plugin;

    element_name = G_OBJECT_TYPE_NAME(self);
    GST_INFO("element_name: '%s'",element_name);

    factory = GST_ELEMENT_GET_CLASS (self)->elementfactory;
    GST_INFO("factory: %p",factory);
    if(factory) {
      plugin_name = GST_PLUGIN_FEATURE (factory)->plugin_name;
      GST_INFO("plugin_name: '%s'",plugin_name);
      plugin = gst_default_registry_find_plugin (plugin_name);
      GST_INFO("plugin: %p",plugin);
      file_name = gst_plugin_get_filename (plugin);
      GST_INFO("file_name: '%s'",file_name);
      /*
      '/home/ensonic/buzztard/lib/gstreamer-0.10/libgstsimsyn.so'
      -> '/home/ensonic/buzztard/share/gstreamer-0.10/GstSimSyn.xml'
      -> '$HOME/.gstreamer-0.10/presets/GstSimSyn.xml'

      '/usr/lib/gstreamer-0.10/libgstaudiofx.so'
      -> '/usr/share/gstreamer-0.10/GstAudioPanorama.xml'
      -> '$HOME/.gstreamer-0.10/presets/GstAudioPanorama.xml'
      */
    }
    
    preset_dir = g_build_filename (g_get_home_dir(), ".gstreamer-0.10", "presets", NULL);
    GST_INFO("preset_dir: '%s'",preset_dir);
    preset_path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.prs", preset_dir, element_name);
    GST_INFO("preset_path: '%s'",preset_path);
    g_mkdir_with_parents (preset_dir, 0755);
    g_free (preset_dir);

	/* attach the preset path to the type */
	g_type_set_qdata (type, preset_path_quark, (gpointer) preset_path);
  }
  return(preset_path);
}

static void
preset_cleanup(gpointer user_data,GObject *self)
{
  GType type = (GType)user_data;
  GList *instances;

  /* remove instance from instance list (if not yet there) */
  instances = (GList *) g_type_get_qdata (type, instance_list_quark);
  if (instances != NULL) {
	instances = g_list_remove (instances, self);
	GST_INFO("old instanc removed");
	g_type_set_qdata (type, instance_list_quark, (gpointer) instances);
  }  
}

static GList*
gst_preset_default_get_preset_names (GstPreset *self)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GList *presets;
  GList *instances;
  GHashTable *preset_meta,*preset_data;
  gboolean found = FALSE;

  /* get the presets from the type */
  if (!preset_get_storage(self,&presets,&preset_meta,&preset_data)) {
	const gchar *preset_path=preset_get_path(self);
    FILE *in;

	GST_DEBUG("probing preset file: '%s'",preset_path);
	
    /* read presets */
    if((in=fopen(preset_path,"rb"))) {
	  const gchar *element_name = G_OBJECT_TYPE_NAME(self);
	  gchar line[LINE_LEN+1],*str,*val;
	  gboolean parse_preset;
	  gchar *preset_name;
	  GHashTable *meta;
      GHashTable *data;
	  GObjectClass *klass;
	  GParamSpec *property;
	  
	  GST_DEBUG("loading preset file: '%s'",preset_path);

      /* read header */
	  if(!fgets(line,LINE_LEN,in)) goto eof_error;
      if(strcmp(line,"GStreamer Preset\n")) {
		GST_WARNING("%s:1: file id expected",preset_path);
		goto eof_error;
	  }
	  if(!fgets(line,LINE_LEN,in)) goto eof_error;
	  /* @todo: what version */
	  if(!fgets(line,LINE_LEN,in)) goto eof_error;
      if(strcmp(g_strchomp(line),element_name)) {
		GST_WARNING("%s:3: wrong element name",preset_path);
		goto eof_error;
	  }
	  if(!fgets(line,LINE_LEN,in)) goto eof_error;
      if(*line!='\n') {
		GST_WARNING("%s:4: blank line expected",preset_path);
		goto eof_error;
	  }
	  
	  klass=G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self));
      
      /* read preset entries*/
	  while(!feof(in)) {
		/* read preset entry */
		fgets(line,LINE_LEN,in);
		g_strchomp(line);
		if(*line) {
		  preset_name=g_strdup(line);
		  GST_INFO("%s: preset '%s'",preset_path,preset_name);
		  
		  data=g_hash_table_new_full(g_str_hash,g_str_equal,NULL,g_free);
		  meta=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
		
          /* read preset lines */
		  parse_preset=TRUE;
		  while(parse_preset) {
			fgets(line,LINE_LEN,in);
			if(feof(in) || (*line=='\n')) {
			  GST_DEBUG("preset done");
			  parse_preset=FALSE;
			  break;
			}
			str=g_strchomp(line);
			while(*str) {
			  if(*str==':') {
				*str='\0';
				GST_DEBUG("meta[%s]='%s'",line,&str[1]);
				if((val=g_hash_table_lookup(meta,line))) {
				  g_free(val);
				  g_hash_table_insert(meta,(gpointer)line,(gpointer)g_strdup(&str[1]));
				}
				else {
				  g_hash_table_insert(meta,(gpointer)g_strdup(line),(gpointer)g_strdup(&str[1]));
				}
				break;
			  }
			  else if(*str=='=') {
				*str='\0';
				GST_DEBUG("data[%s]='%s'",line,&str[1]);
				if((property=g_object_class_find_property(klass,line))) {
				  g_hash_table_insert(data,(gpointer)property->name,(gpointer)g_strdup(&str[1]));
			    }
				else {
				  GST_WARNING("%s: Invalid property '%s'",preset_path,line);
				}
				break;
			  }
			  str++;
			}
			/* @todo: handle childproxy properties
			 * <property>[child]=<value>
			 */			
		  }
		  
		  GST_INFO("preset: %p, %p",meta,data);
		  g_hash_table_insert(preset_data,(gpointer)preset_name,(gpointer)data);
		  g_hash_table_insert(preset_meta,(gpointer)preset_name,(gpointer)meta);
		  presets=g_list_insert_sorted(presets,(gpointer)preset_name,(GCompareFunc)strcmp);
		}
	  }

eof_error:
      fclose(in);
    }
    else {
      GST_INFO("can't open preset file: '%s'",preset_path);
    }

    /* attach the preset to the type */
	g_type_set_qdata (type, preset_list_quark, (gpointer) presets);
  }
  
  /* insert instance in instance list (if not yet there) */
  instances = (GList *) g_type_get_qdata (type, instance_list_quark);
  if (instances != NULL) {
	if (g_list_find (instances, self))
	  found = TRUE;
  }
  if (!found) {
	GST_INFO("new instance added");
	/* register a weak ref, to clean up when the object gets destroyed */
	g_object_weak_ref(G_OBJECT(self),preset_cleanup,(gpointer)type);
	instances = g_list_prepend (instances, self);
	g_type_set_qdata (type, instance_list_quark, (gpointer) instances);
  }  
  return(presets);
}

static gboolean
gst_preset_default_load_preset (GstPreset *self, const gchar *name)
{
  GList *presets;
  GHashTable *preset_data;
  
  /* get the presets from the type */
  if(preset_get_storage(self,&presets,NULL,&preset_data)) {
	GList *node;

	if((node = g_list_find_custom (presets, name, (GCompareFunc)strcmp))) {
	  GHashTable *data=g_hash_table_lookup(preset_data,node->data);
	  GParamSpec **properties,*property;
	  GType base,parent;
	  guint i,number_of_properties;
	  gchar *val=NULL;
	  
	  GST_DEBUG("loading preset : '%s', data : %p (size=%d)",name,data,g_hash_table_size(data));

	  /* preset found, now set values */
	  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self)),&number_of_properties))) {
		for(i=0;i<number_of_properties;i++) {
		  property=properties[i];
		  /* skip non-controlable */
		  if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
		  /* check if we have a settings for this property */
		  if((val=(gchar *)g_hash_table_lookup(data,property->name))) {
			GST_DEBUG("setting value '%s' for property '%s'",val,property->name);
			/* get base type */
			base = property->value_type;
			while ((parent = g_type_parent (base))) {
			  base = parent;
			}			
			switch(base) {
			  case G_TYPE_INT:
			  case G_TYPE_UINT:
			  case G_TYPE_BOOLEAN:
			  case G_TYPE_ENUM:
				g_object_set(G_OBJECT(self),property->name,atoi(val),NULL);
				break;
			  case G_TYPE_LONG:
			  case G_TYPE_ULONG:
				g_object_set(G_OBJECT(self),property->name,atol(val),NULL);
				break;
			  case G_TYPE_FLOAT:
				g_object_set(G_OBJECT(self),property->name,(float)g_ascii_strtod(val,NULL),NULL);
				break;
			  case G_TYPE_DOUBLE:
				g_object_set(G_OBJECT(self),property->name,g_ascii_strtod(val,NULL),NULL);
				break;
			  case G_TYPE_STRING:
				g_object_set(G_OBJECT(self),property->name,val,NULL);
				break;
			  default:
				GST_WARNING ("incomplete implementation for GParamSpec type '%s'",
                  G_PARAM_SPEC_TYPE_NAME (property));
			}
		  }
		  else {
			GST_INFO("parameter '%s' not in preset",property->name);
		  }
		}
		return(TRUE);
	  }
	}
  }
  else {
	GST_INFO("no presets");
  }
  return(FALSE);
}

static void
preset_store_meta(gpointer key, gpointer value, gpointer user_data)
{
  if(key && value) {
	fprintf((FILE *)user_data,"%s:%s\n",(gchar*)key,(gchar*)value);
  }
}

static void
preset_store_data(gpointer key, gpointer value, gpointer user_data)
{
  if(key && value) {
	fprintf((FILE *)user_data,"%s=%s\n",(gchar*)key,(gchar*)value);
  }
}

static gboolean
gst_preset_default_save_presets_file (GstPreset *self)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  gboolean res=FALSE;
  GList *presets;
  GHashTable *preset_meta,*preset_data;
  
  /* get the presets from the type */
  if (preset_get_storage(self,&presets,&preset_meta,&preset_data)) {
	const gchar *preset_path=preset_get_path(self);
	FILE *out;
	
	GST_DEBUG("saving preset file: '%s'",preset_path);

	/* @todo create backup */
	
	/* write presets */
	if((out=fopen(preset_path,"wb"))) {
	  const gchar *element_name = G_OBJECT_TYPE_NAME(self);
	  gchar *preset_name;
	  GList *node;
	  GHashTable *meta,*data;
	  
	  /* write header */
	  if(!(fputs("GStreamer Preset\n",out))) goto eof_error;
	  /* @todo: what version */
	  if(!(fputs("1.0\n",out))) goto eof_error;
	  if(!(fputs(element_name,out))) goto eof_error;
	  if(!(fputs("\n\n",out))) goto eof_error;
		
	  /* write preset entries*/
	  for(node=presets;node;node=g_list_next(node)) {
		preset_name=node->data;
		/* write preset entry*/
		if(!(fputs(preset_name,out))) goto eof_error;
		if(!(fputs("\n",out))) goto eof_error;
		
		/* write data */
		meta=g_hash_table_lookup(preset_meta,(gpointer)preset_name);
		g_hash_table_foreach(meta,preset_store_meta,out);
		data=g_hash_table_lookup(preset_data,(gpointer)preset_name);
		g_hash_table_foreach(data,preset_store_data,out);
		if(!(fputs("\n",out))) goto eof_error;
	  }
  
	  res=TRUE;
eof_error:
	  fclose(out);
	}
  }
  else {
	GST_INFO("no presets");
  }
  return(res);
}

static gboolean
gst_preset_default_save_preset (GstPreset *self, const gchar *name)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GList *presets;
  GHashTable *preset_meta,*preset_data;
  GHashTable *meta,*data;
  GParamSpec **properties,*property;
  GType base,parent;
  guint i,number_of_properties;
  gchar *str=NULL,buffer[30+1];
  /*guint flags;*/
  
  GST_INFO("saving new preset: %s",name);

  /* get the presets from the type */
  preset_get_storage(self,&presets,&preset_meta,&preset_data);
	
  data=g_hash_table_new_full(g_str_hash,g_str_equal,NULL,g_free);
  meta=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);

  /* take copies of current gobject properties from self */
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self)),&number_of_properties))) {
	for(i=0;i<number_of_properties;i++) {
	  property=properties[i];
	  /*flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));*/
	  
	  /* skip non-controlable */
	  if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
	  /*
	  else if(!(flags&GST_PROPERTY_META_STATE)) continue;
	  else if(voice_class && g_object_class_find_property(voice_class,property->name)) continue;
	  */

	  /* get base type */
	  base = property->value_type;
	  while ((parent = g_type_parent (base))) {
		base = parent;
	  }			
	  /* get value and serialize */
	  GST_INFO("  storing property: %s (type is %s)",property->name,g_type_name(base));
	  switch(base) {
		case G_TYPE_BOOLEAN:
		case G_TYPE_ENUM:
		case G_TYPE_INT: {
		  gint val;
		  
		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  str=g_strdup_printf("%d",val);
		} break;
		case G_TYPE_UINT: {
		  guint val;
		  
		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  str=g_strdup_printf("%u",val);
		} break;
		case G_TYPE_LONG: {
		  glong val;
		  
		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  str=g_strdup_printf("%ld",val);
		} break;
		case G_TYPE_ULONG: {
		  gulong val;
		  
		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  str=g_strdup_printf("%lu",val);
		} break;
		case G_TYPE_FLOAT: {
		  gfloat val;

		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  g_ascii_dtostr(buffer,30,(gdouble)val);
		  str=g_strdup(buffer);
		} break;
		case G_TYPE_DOUBLE: {
		  gdouble val;

		  g_object_get(G_OBJECT(self),property->name,&val,NULL);
		  g_ascii_dtostr(buffer,30,val);
		  str=g_strdup(buffer);
		} break;
		case G_TYPE_STRING:
		  g_object_get(G_OBJECT(self),property->name,&str,NULL);
		  if(str && !*str) str=NULL;
		  break;
		default:
		  GST_WARNING ("incomplete implementation for GParamSpec type '%s'",
			G_PARAM_SPEC_TYPE_NAME (property));
	  }
	  if(str) {
		g_hash_table_insert(data,(gpointer)property->name,(gpointer)str);
		str=NULL;
	  }
	  
	}
	GST_INFO("  saved");
  }
  
  /*
   * flock(fileno())
   * http://www.ecst.csuchico.edu/~beej/guide/ipc/flock.html
   */
  g_hash_table_insert(preset_data,(gpointer)name,(gpointer)data);
  g_hash_table_insert(preset_meta,(gpointer)name,(gpointer)meta);
  presets=g_list_insert_sorted(presets,(gpointer)name,(GCompareFunc)strcmp);
  /* attach the preset list to the type */
  g_type_set_qdata (type, preset_list_quark, (gpointer) presets);
  GST_INFO("done");

  return(gst_preset_default_save_presets_file(self));
}

static gboolean
gst_preset_default_rename_preset (GstPreset *self, const gchar *old_name, const gchar *new_name)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GList *presets;
  GHashTable *preset_meta,*preset_data;
  
  /* get the presets from the type */
  if(preset_get_storage(self,&presets,&preset_meta,&preset_data)) {
	GList *node;

	if((node = g_list_find_custom (presets, old_name, (GCompareFunc)strcmp))) {
	  GHashTable *meta,*data;

	  /* readd under new name */
	  presets = g_list_delete_link (presets, node);
	  presets = g_list_insert_sorted (presets, (gpointer)new_name, (GCompareFunc)strcmp);

	  /* readd the hash entries */
	  if((meta=g_hash_table_lookup(preset_meta,node->data))) {
		g_hash_table_remove(preset_meta,node->data);
		g_hash_table_insert(preset_meta,(gpointer)new_name,(gpointer)meta);
	  }
	  if((data=g_hash_table_lookup(preset_data,node->data))) {
		g_hash_table_remove(preset_data,node->data);
		g_hash_table_insert(preset_data,(gpointer)new_name,(gpointer)data);
	  }

	  GST_INFO ("preset moved 's' -> '%s'", old_name, new_name);
	  g_type_set_qdata (type, preset_list_quark, (gpointer) presets);
	  
	  return(gst_preset_default_save_presets_file(self));
	}
  }
  else {
	GST_WARNING("no presets");
  }
  return(FALSE);
}

static gboolean
gst_preset_default_delete_preset (GstPreset *self, const gchar *name)
{
  GType type = G_TYPE_FROM_INSTANCE (self);
  GList *presets;
  GHashTable *preset_meta,*preset_data;
  
  /* get the presets from the type */
  if(preset_get_storage(self,&presets,&preset_meta,&preset_data)) {
	GList *node;

	if((node = g_list_find_custom (presets, name, (GCompareFunc)strcmp))) {
	  GHashTable *meta,*data;
  
	  /* remove the found one */
	  presets = g_list_delete_link (presets, node);
	  
	  /* free the hash entries */
	  if((meta=g_hash_table_lookup(preset_meta,node->data))) {
		g_hash_table_remove(preset_meta,node->data);
		g_hash_table_destroy(meta);
	  }
	  if((data=g_hash_table_lookup(preset_data,node->data))) {
		g_hash_table_remove(preset_data,node->data);
		g_hash_table_destroy(data);
	  }
	  
	  GST_INFO ("preset removed 's'", name);
	  g_type_set_qdata (type, preset_list_quark, (gpointer) presets);
	  g_free ((gpointer)name);
  
	  return( gst_preset_default_save_presets_file (self));
	}
  }
  else {
	GST_WARNING("no presets");
  }
  return(FALSE);
}

static gboolean 
gst_preset_default_set_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar *value)
{
  gboolean res=FALSE;
  GList *presets;
  GHashTable *preset_meta;
  
  /* get the presets from the type */
  if(preset_get_storage(self,&presets,&preset_meta,NULL)) {
	GList *node;
	
	if((node=g_list_find_custom(presets,name,(GCompareFunc)strcmp))) {
	  GHashTable *meta=g_hash_table_lookup(preset_meta,node->data);
      gchar *old_value;
      gboolean changed=FALSE;
      
      if((old_value=g_hash_table_lookup(meta,tag))) {
        g_free(old_value);
        changed=TRUE;
      }
      if(value) {
		if(changed) tag=g_strdup(tag);
        g_hash_table_insert(meta,(gpointer)tag,g_strdup(value));
        changed=TRUE;
      }
      if(changed) {
        res=gst_preset_default_save_presets_file(self);
      }
	}
  }
  else {
	GST_WARNING("no presets");
  }
  return(res);
}

static gboolean
gst_preset_default_get_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar **value)
{
  gboolean res=FALSE;
  GList *presets;
  GHashTable *preset_meta;
  
  /* get the presets from the type */
  if(preset_get_storage(self,&presets,&preset_meta,NULL)) {
	GList *node;
	
	if((node=g_list_find_custom(presets,name,(GCompareFunc)strcmp))) {
	  GHashTable *meta=g_hash_table_lookup(preset_meta,node->data);
      gchar *new_value;

      if((new_value=g_hash_table_lookup(meta,tag))) {
        *value=g_strdup(new_value);
        res=TRUE;
      }
	}
  }
  else {
	GST_WARNING("no presets");
  }
  if(!res) *value=NULL;
  return(res);
}

static void
gst_preset_default_create_preset (GstPreset *self)
{
  GParamSpec **properties,*property;
  guint i,number_of_properties;
  GType param_type,base_type;

  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_OBJECT_GET_CLASS(self)),&number_of_properties))) {
    gdouble rnd;
    guint flags=0;

    /* @todo: what about voice properties */
    
    GST_INFO("nr of values : %d", number_of_properties);
    for(i=0;i<number_of_properties;i++) {
      property=properties[i];
      if(GST_IS_PROPERTY_META(self)) {
        flags=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
      }

      /* skip non-controlable, trigger params & voice params */
      if(!(property->flags&GST_PARAM_CONTROLLABLE)) continue;
      else if(!(flags&GST_PROPERTY_META_STATE)) continue;

      GST_INFO("property '%s' (GType=%d)",property->name,property->value_type);

      param_type=property->value_type;
      while((base_type=g_type_parent(param_type))) param_type=base_type;
      
      rnd=((gdouble)rand()) / (RAND_MAX + 1.0);
      switch(param_type) {
        case G_TYPE_BOOLEAN: {
          g_object_set(self,property->name,(gboolean)(2.0 * rnd),NULL);
        } break;
        case G_TYPE_INT: {
          const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
          
          g_object_set(self,property->name,(gint)(int_property->minimum+((int_property->maximum-int_property->minimum)*rnd)),NULL);
        } break;
        case G_TYPE_UINT: {
          const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
          
          g_object_set(self,property->name,(guint)(uint_property->minimum+((uint_property->maximum-uint_property->minimum)*rnd)),NULL);
        }  break;
        case G_TYPE_DOUBLE: {
          const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);

          g_object_set(self,property->name,(gdouble)(double_property->minimum+((double_property->maximum-double_property->minimum) * rnd)),NULL);
        } break;
        case G_TYPE_ENUM: {
          const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
          const GEnumClass *enum_class=enum_property->enum_class;

          g_object_set(self,property->name,(gulong)(enum_class->minimum+((enum_class->maximum-enum_class->minimum) * rnd)),NULL);
        } break;
        default:
          //GST_WARNING("unhandled GType=%d:'%s'",param_type,G_VALUE_TYPE_NAME(param_type));
          GST_WARNING("unhandled GType=%d",param_type);
      }
    }
  }
}

/* wrapper */

/**
 * gst_preset_get_preset_names:
 * @self: a #GObject that implements #GstPreset
 *
 * Get a copy of the preset list names. Free list when done.
 *
 * Returns: list with names
 */
GList*
gst_preset_get_preset_names (GstPreset *self)
{
  g_return_val_if_fail (GST_IS_PRESET (self), NULL);

  return (GST_PRESET_GET_INTERFACE (self)->get_preset_names (self));  
}

/**
 * gst_preset_load_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to load
 *
 * Load the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_load_preset (GstPreset *self, const gchar *name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->load_preset (self, name));  
}

/**
 * gst_preset_save_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to save
 *
 * Save the current preset under the given name. If there is already a preset by
 * this @name it will be overwritten.
 *
 * Returns: %TRUE for success, %FALSE
 */
gboolean
gst_preset_save_preset (GstPreset *self, const gchar *name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->save_preset (self, name));  
}

/**
 * gst_preset_rename_preset:
 * @self: a #GObject that implements #GstPreset
 * @old_name: current preset name
 * @new_name: new preset name
 *
 * Renames a preset. If there is already a preset by thr @new_name it will be
 * overwritten.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with @old_name
 */
gboolean
gst_preset_rename_preset (GstPreset *self, const gchar *old_name, const gchar *new_name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (old_name, FALSE);
  g_return_val_if_fail (new_name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->rename_preset (self,old_name,new_name));  
}

/**
 * gst_preset_delete_preset:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name to remove
 *
 * Delete the given preset.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_delete_preset (GstPreset *self, const gchar *name)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);

  return (GST_PRESET_GET_INTERFACE (self)->delete_preset (self,name));  
}

/**
 * gst_preset_set_meta:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name
 * @tag: meta data item name
 * @value: new value
 *
 * Sets a new @value for an existing meta data item or adds a new item. Meta
 * data @tag names can be something like e.g. "comment". Supplying %NULL for the
 * @value will unset an existing value.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 */
gboolean
gst_preset_set_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar *value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);

  GST_PRESET_GET_INTERFACE (self)->set_meta (self,name,tag,value);
}

/**
 * gst_preset_get_meta:
 * @self: a #GObject that implements #GstPreset
 * @name: preset name
 * @tag: meta data item name
 * @value: value
 *
 * Gets the @value for an existing meta data @tag. Meta data @tag names can be
 * something like e.g. "comment". Returned values need to be released when done.
 *
 * Returns: %TRUE for success, %FALSE if e.g. there is no preset with that @name
 * or no value for the given @tag
 */
gboolean
gst_preset_get_meta (GstPreset *self,const gchar *name, const gchar *tag, gchar **value)
{
  g_return_val_if_fail (GST_IS_PRESET (self), FALSE);
  g_return_val_if_fail (name, FALSE);
  g_return_val_if_fail (tag, FALSE);
  g_return_val_if_fail (value, FALSE);

  GST_PRESET_GET_INTERFACE (self)->get_meta (self,name,tag,value);
}

/**
 * gst_preset_create_preset:
 * @self: a #GObject that implements #GstPreset
 *
 * Create a new randomized preset. This method is optional. If not implemented
 * true randomization will be applied.
 */
void
gst_preset_create_preset (GstPreset *self)
{
  g_return_if_fail (GST_IS_PRESET (self));

  GST_PRESET_GET_INTERFACE (self)->create_preset (self);
}

/* class internals */

static void
gst_preset_class_init(GstPresetInterface *iface)
{
  iface->get_preset_names = gst_preset_default_get_preset_names;

  iface->load_preset = gst_preset_default_load_preset;
  iface->save_preset = gst_preset_default_save_preset;
  iface->rename_preset = gst_preset_default_rename_preset;
  iface->delete_preset = gst_preset_default_delete_preset;
  
  iface->set_meta = gst_preset_default_set_meta;
  iface->get_meta = gst_preset_default_get_meta;
  
  iface->create_preset = gst_preset_default_create_preset;
}

static void
gst_preset_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* init default implementation */
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "preset", GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "preset interface");

    /* create quarks for use with g_type_{g,s}et_qdata() */
    preset_list_quark=g_quark_from_string("GstPreset::presets");
    preset_path_quark=g_quark_from_string("GstPreset::path");
	preset_data_quark=g_quark_from_string("GstPreset::data");
	preset_meta_quark=g_quark_from_string("GstPreset::meta");
    instance_list_quark=g_quark_from_string("GstPreset::instances");

    initialized = TRUE;
  }
}

GType
gst_preset_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstPresetInterface),
      (GBaseInitFunc) gst_preset_base_init,   /* base_init */
      NULL,   /* base_finalize */
      (GClassInitFunc) gst_preset_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"GstPreset",&info,0);
  }
  return type;
}
