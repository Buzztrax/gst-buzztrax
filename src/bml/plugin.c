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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

// see http://bugzilla.gnome.org/show_bug.cgi?id=570233
//#define BUILD_STRUCTURE 1
#ifdef BUILD_STRUCTURE
GstStructure *bml_meta_all;
#endif

GstPlugin *bml_plugin;
GHashTable *bml_descriptors_by_element_type;
GHashTable *bml_descriptors_by_voice_type;
GHashTable *bml_voice_type_by_element_type;
GHashTable *bml_help_uri_by_descriptor;
GHashTable *bml_preset_path_by_descriptor;
GHashTable *bml_category_by_machine_name;

GQuark gst_bml_property_meta_quark_type;

#if HAVE_BMLW
extern gboolean bmlw_describe_plugin(gchar *pathname, gpointer bmh);
extern gboolean bmlw_gstbml_register_element(GstPlugin *plugin, GstStructure *bml_meta);
#endif
extern gboolean bmln_describe_plugin(gchar *pathname, gpointer bmh);
extern gboolean bmln_gstbml_register_element(GstPlugin *plugin, GstStructure *bml_meta);

typedef int (*bsearchcomparefunc)(const void *,const void *);


static gboolean read_index(const gchar *dir_name) {
  gchar *file_name;
  FILE *file;
  gboolean res=FALSE;
  
  /* we want a GHashTable bml_category_by_machine_name
   * with the plugin-name (BM_PROP_SHORT_NAME) as a key
   * and the categories as values.
   *
   * this can be used in utils.c: gstbml_class_set_details()
   *
   * maybe we can also filter some plugins based on categories
   */

  file_name=g_build_filename(dir_name,"index.txt",NULL);
  if((file=fopen(file_name,"rt"))) {
    GST_INFO("found buzz machine index at \"%s\"",file_name);
     gchar line[500],*entry;
     gchar categories[1000]="";
     gint cat_pos=0,i,len;
    
    /* the format
     * there are:
     *   comments?  : ","
     *   level-lines: "1,-----"
     *   categories : "/Peer Control"
     *   end-of-cat.: "/.."
     *   separators : "-----" or "--- Dx ---"
     *   machines   : "Arguelles Pro3"
     *   mach.+alias: "Argüelles Pro2, Arguelles Pro2"
     */
     
    while(!feof(file)) {
      if(fgets(line,500,file)) {
        // strip leading and trailing spaces and convert
        entry=g_convert(g_strstrip(line),-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
        if(entry[0]=='/') {
          if(entry[1]=='.' && entry[2]=='.') {
            // pop stack
            if(cat_pos>0) {
              while(cat_pos>0 && categories[cat_pos]!='/')
                cat_pos--;
              categories[cat_pos]='\0';
            }
            GST_DEBUG("- %4d %s",cat_pos,categories);
          }
          else {
            // push stack
            len=strlen(entry);
            if((cat_pos+len)<1000) {
              categories[cat_pos++]='/';
              for(i=1;i<len;i++) {
                categories[cat_pos++]=(entry[i]!='/')?entry[i]:'+';
              }
              categories[cat_pos]='\0';
            }
            GST_DEBUG("+ %4d %s",cat_pos,categories);
          }
        }
        else {
          if(entry[0]=='-' || entry[0]==',' || g_ascii_isdigit(entry[0])) {
            // skip for now
          }
          else if (g_ascii_isalpha(entry[0])) {
            // machines
            // check for "'" and cut the alias
            gchar **names=g_strsplit(entry,",",-1);
            gchar *cat=g_strdup(categories),*beg,*end;
            gint a;
            
            // we need to filter 'Generators,Effects,Gear' from the categories
            if ((beg=strstr(cat,"/Generator"))) {
              end=&beg[strlen("/Generator")];
              memmove(beg,end,strlen(end)+1);
            }
            if ((beg=strstr(cat,"/Effect"))) {
              end=&beg[strlen("/Effect")];
              memmove(beg,end,strlen(end)+1);
            }
            if ((beg=strstr(cat,"/Gear"))) {
              end=&beg[strlen("/Gear")];
              memmove(beg,end,strlen(end)+1);
            }

            for(a=0;a<g_strv_length(names);a++) {
              if(names[a] && *names[a]) {
                GST_DEBUG("  %s -> %s",names[a],categories);
                 g_hash_table_insert(bml_category_by_machine_name,g_strdup(names[a]),g_strdup(cat));
              }
            }
            g_free(cat);
            g_strfreev(names);
          }
        }
        g_free(entry);
      }
    }
    
    res=TRUE;
    fclose(file);
  }
  g_free(file_name);
  return(res);
}

static int blacklist_compare(const void *node1, const void *node2) {
  //GST_DEBUG("comparing '%s' '%s'",node1,*(gchar**)node2);
  return (strcasecmp((gchar *)node1,*(gchar**)node2));
}

static const gchar *get_bml_path(void) {
#if HAVE_BMLW
  return(LIBDIR G_DIR_SEPARATOR_S "Gear:" LIBDIR G_DIR_SEPARATOR_S "Gear" G_DIR_SEPARATOR_S "Generators:" LIBDIR G_DIR_SEPARATOR_S "Gear" G_DIR_SEPARATOR_S "Effects");
#else
  return(LIBDIR G_DIR_SEPARATOR_S "Gear");
#endif
}

/*
 * dir_scan:
 *
 * Search the given directory for plugins. Supress some based on a built-in
 * blacklist.
 */
static gboolean dir_scan(const gchar *dir_name) {
  GDir *dir;
  gchar *file_name,*ext;
  const gchar *entry_name;
  gpointer bmh;
  gboolean res=FALSE;

  const gchar *blacklist[] ={
    "2NDPROCESS NUMBIRD 1.4.DLL",
    "ANEURYSM DISTGARB.DLL",
    "ARGUELLES FX2.DLL",
    /*"ARGUELLES PRO3.DLL",
     * is disguised as an effect (stereo)
     * I patched @2d28 : 02 00 00 00 0c 00 00 00 -> 01 00 ...
     * segfault still :/
     * bml ../gstbmlsrc.c:512:gst_bml_src_create_stereo:   calling work_m2s(5292)
     */
    "ARGUELLES ROVOX.DLL",
    "AUTOMATON COMPRESSOR MKII.DLL",
    /*"AUTOMATON PARAMETRIC EQ.DLL",
      - crashes on CMachineInterface::SetNumTracks(1)
      */
    "AUTOMATON VCF.DLL",
    "BUZZINAMOVIE.DLL",
    /*"CHEAPO SPREAD.DLL",
      - hangs after GetInfo(), should be fixed after installing msvcr70.dll and
        with new function implemented 
      */
    "CHIMP REPLAY.DLL",
    "CYANPHASE AUXRETURN.DLL",
    "CYANPHASE DMO EFFECT ADAPTER.DLL",
    "CYANPHASE SEA CUCUMBER.DLL",
    "CYANPHASE SONGINFO.DLL",
    "CYANPHASE UNNATIVE EFFECTS.DLL",
    "CYANPHASE VIDIST 01.DLL",
    "DAVE'S SMOOTHERDRIVE.DLL",
    "DEX CROSSFADE.DLL",
    "DEX EFX.DLL",
    "DEX RINGMOD.DLL",
    "DT_BLOCKFX (STEREO).DLL",
    "DT_BLOCKFX.DLL",
    "FIRESLEDGE ANTIOPE-1.DLL",
    "FREQUENCY UNKNOWN FREQ OUT.DLL",
    "FUZZPILZ POTTWAL.DLL",
    "FUZZPILZ RO-BOT.DLL",
    "FUZZPILZ UNWIELDYDELAY3.DLL",
    "GAZBABY'S PROLOGIC ENCODER.DLL",
    "GEONIK'S VISUALIZATION.DLL",
    "HD F-FLANGER.DLL",
    "HD MOD-X.DLL",
    "J.R. BP V1 FILTER.DLL",
    "J.R. BP V2 FILTER.DLL",
    "J.R. HP FILTER.DLL",
    "J.R. LP FILTER.DLL",
    "J.R. NOTCH FILTER.DLL",
    "JESKOLA AUXSEND.DLL",
    "JESKOLA EQ-3 XP.DLL",
    "JESKOLA MULTIPLIER.DLL",
    /*"JESKOLA REVERB 2.DLL",*/
    /*"JESKOLA REVERB.DLL",*/
    /*"JESKOLA STEREO REVERB.DLL",*/
    "JESKOLA WAVE SHAPER.DLL",
    "JOASMURF VUMETER.DLL",
    "JOYPLUG 1.DLL",
    "LD AUXSEND.DLL",
    "LD FMFILTER.DLL",
    "LD GATE.DLL",
    "LD MIXER.DLL",
    "LD VOCODER XP.DLL",
    "LD VOCODER.DLL",
    "LOST_BIT IMOD.DLL",
    "LOST_BIT IPAN.DLL",
    "MIMO'S MIDIOUT.DLL",
    "MIMO'S MIXO.DLL",
    "NINEREEDS 2P FILTER.DLL",
    "NINEREEDS DISCRETIZE.DLL",
    "NINEREEDS DISCRITIZE.DLL",
    "NINEREEDS FADE.DLL",
    "NINEREEDS LFO FADE.DLL",
    "PITCHWIZARD.DLL",
    "POLAC MIDI IN.DLL",
    "POLAC VST 1.1.DLL",
    "PSI KRAFT.DLL",
    "PSI KRAFT2.DLL",
    "REPEATER.DLL",
    "ROUT EQ-10.DLL",
    "ROUT VST PLUGIN LOADER.DLL",
    "SHAMAN CHORUS.DLL",
    "STATIC DUAFILT II.DLL",
      // *** glibc detected *** /home/ensonic/projects/buzztard/bml/src/.libs/lt-bmltest_info: free(): invalid next size (normal): 0x0805cc18 ***
    "TRACK ORGANIZER.DLL",
    "VGRAPHITY.DLL",
    "WHITENOISE AUXRETURN.DLL",
    "XMIX.DLL",
    "YNZN'S AMPLITUDE MODULATOR.DLL",
    "YNZN'S CHIRPFILTER.DLL",
    "YNZN'S MULTIPLEXER.DLL",
    "YNZN'S REMOTE COMPRESSOR.DLL",
    "YNZN'S REMOTE GATE.DLL",
    "YNZN'S VOCODER.DLL",
    "ZEPHOD BLUE FILTER.DLL",
    "ZEPHOD GREEN FILTER.DLL",
    "ZEPHOD_RESAW.DLL",
    "ZU ?TAPS.DLL",
    "ZU µTAPS.DLL"
  };

  GST_INFO("scanning directory \"%s\"",dir_name);

  dir = g_dir_open (dir_name, 0, NULL);
  if (!dir) return(res);

  while((entry_name=g_dir_read_name(dir))) {
    ext=strrchr(entry_name,'.');
    if (ext && (!strcasecmp(ext,".dll") || !strcmp(ext,".so"))) {
      /* test against blacklist */
      if(!bsearch(entry_name, blacklist, G_N_ELEMENTS(blacklist),sizeof(gchar *), blacklist_compare)) {
        file_name=g_build_filename (dir_name, entry_name, NULL);
        //file_name=g_strconcat(dir_name, G_DIR_SEPARATOR_S, entry_name, NULL);
        GST_WARNING("trying plugin '%s','%s'",entry_name,file_name);
        if(!strcasecmp(ext,".dll")) {
#if HAVE_BMLW
          if((bmh=bmlw_open(file_name))) {
            if(bmlw_describe_plugin(file_name,bmh)) {
              res=TRUE;
            }
            /* @todo: free here, or leave instance open to be used in class init */
#ifdef BUILD_STRUCTURE
            /* once we switch to the ifdefs, move the close() down and remove else {} */ 
            bmlw_close(bmh);
#endif
          }
          else {
            GST_WARNING("machine %s could not be loaded",file_name);
          }
#else
          GST_WARNING("no dll emulation on non x86 platforms");
#endif
        }
        else {
          if((bmh=bmln_open(file_name))) {
            if(bmln_describe_plugin(file_name,bmh)) {
              res=TRUE;
            }
            /* @todo: free here, or leave instance open to be used in class init */
#ifdef BUILD_STRUCTURE
            /* once we switch to the ifdefs, move the close() down and remove else {} */ 
            bmln_close(bmh);
#endif
          }
          else {
            GST_WARNING("machine %s could not be loaded",file_name);
          }
        }
        g_free(file_name);
      }
      else {
        GST_WARNING("machine %s is black-listed",entry_name);
      }
    }
  }
  g_dir_close (dir);
  
  GST_INFO("after scanning dir \"%s\", res=%d",dir_name,res);
  return(res);
}

/*
 * bml_scan:
 *
 * Search through the $(BML_PATH) (or a default path) for any buzz plugins. Each
 * plugin library is tested using dlopen() and dlsym(,"bml_descriptor").
 * After loading each library, the callback function is called to process it.
 * This function leaves items passed to the callback function open.
 */

static gboolean bml_scan(void) {
  const gchar *bml_path;
  gchar **paths;
  gint i,path_entries;
  gboolean res=FALSE;

  bml_path = g_getenv("BML_PATH");

  if(!bml_path || !*bml_path) {
    bml_path = get_bml_path();
    GST_WARNING("You do not have a BML_PATH environment variable set, using default: '%s'", bml_path);
  }
  
  paths=g_strsplit(bml_path,G_SEARCHPATH_SEPARATOR_S,0);
  path_entries=g_strv_length(paths);
  GST_INFO("%d dirs in search paths \"%s\"",path_entries,bml_path);
  
  // check of index.txt in any of the paths
  for(i=0;i<path_entries;i++) {
    if(read_index(paths[i]))
      break;
  }

  for(i=0;i<path_entries;i++) {
    res|=dir_scan(paths[i]);
  }
  g_strfreev(paths);

  GST_INFO("after scanning path \"%s\", res=%d",bml_path,res);
  return(res);
}

static gboolean plugin_init (GstPlugin * plugin) {
  gboolean res;

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bml", GST_DEBUG_FG_GREEN | GST_DEBUG_BG_BLACK | GST_DEBUG_BOLD, "BML");

  GST_INFO("lets go ===========================================================");
  
  oil_init ();
  
#if GST_CHECK_VERSION(0,10,22)
  gst_plugin_add_dependency_simple (plugin, 
    "BML_PATH",
    get_bml_path(),
    "so,dll",
    GST_PLUGIN_DEPENDENCY_FLAG_PATHS_ARE_DEFAULT_ONLY|GST_PLUGIN_DEPENDENCY_FLAG_FILE_NAME_IS_SUFFIX);
#endif

  // init bml library
  if(!bml_setup(0)) {
    GST_WARNING("failed to init bml library");
    return(FALSE);
  }

  // init global data
  // we have to keep help_uri & preset_path loaded for whole time, as the
  // strings can be accessed at any time (that includes after plugin_init()
  bml_descriptors_by_element_type=g_hash_table_new(NULL, NULL);
  bml_descriptors_by_voice_type=g_hash_table_new(NULL, NULL);
  bml_voice_type_by_element_type=g_hash_table_new(NULL, NULL);
  bml_help_uri_by_descriptor=g_hash_table_new(NULL, NULL);
  bml_preset_path_by_descriptor=g_hash_table_new(NULL, NULL);
  bml_category_by_machine_name=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  bml_plugin=plugin;

  // @todo this is a hack
#if HAVE_BMLW
  bmlw_set_master_info(120,4,44100);
#endif
  bmln_set_master_info(120,4,44100);

  /* initialize gst controller library */
  gst_controller_init(NULL, NULL);

  gst_bml_property_meta_quark_type=g_quark_from_string("GstBMLPropertyMeta::type");

  GST_INFO("scan for plugins");
  /* we need a mechanism in the registry (or an own registry) to avoid scanning
   * when not building/updating a registry
   * see http://bugzilla.gnome.org/show_bug.cgi?id=570233
   * until that implemneted there, we could easily store the structure in a
   * extra file ourself 
   *
   * Plugin init:
   * At this point we would request the cache data. If its NULL, we scan.
   * During scanning we would build the cache data.
   *
   * Registering types:
   * In any case we now have the cache data and  we iterate over it
   * (a hierarchical structure with the plugin name as major keys) and call
   * bml?_describe_plugin(...) for each entry. _describe_plugin() registers the
   * gobject type. It needs to be changed to take the parameters from the cache
   * data.
   *
   * FIXME: check if we can get tid of the hashtables once this all works
   * 
   * FIXME: defer dll loading more
   * utils:gstbml_class_base_init() loads the dll now, unfortunately
   * gst_element_register() refs the class, we need to cache more info (almost
   * the full machine-info :/
   *
   */
#ifdef BUILD_STRUCTURE
  {
    // we get this from the registry later on
    gchar *registry_filename=g_build_filename(g_get_home_dir(),".gstreamer-0.10/bml-registry.bin",NULL);
    gchar *bml_meta_str;
    gint n;

    if(g_file_get_contents(registry_filename,&bml_meta_str,NULL,NULL)) {
      /* FIXME: this does not deserialize correctly */
      bml_meta_all=gst_structure_from_string(bml_meta_str,NULL);
      g_free(bml_meta_str);
    }
    else {
      bml_meta_all=gst_structure_empty_new("bml");
    }
    n=gst_structure_n_fields(bml_meta_all);
    GST_INFO("%d entries in cache",n);
    if(!n) {
#endif
      res=bml_scan();
#ifdef BUILD_STRUCTURE
      if(res) {
        n=gst_structure_n_fields(bml_meta_all);
        GST_INFO("%d entries after scanning",n);
        bml_meta_str=gst_structure_to_string(bml_meta_all);
        g_file_set_contents(registry_filename,bml_meta_str,-1,NULL);
        g_free(bml_meta_str);
      }
    }
    else {
      res=TRUE;
    }
    g_free(registry_filename);
    
    {
      // dump structure
      gint i;
      const gchar *name;
      const GValue *value;
      gchar *desc;
      GQuark bmln_type=g_quark_from_static_string("bmln");
      
      printf("%3d entries\n",n);
      
      for(i=0;i<n;i++) {
        name=gst_structure_nth_field_name(bml_meta_all,i); 
        value=gst_structure_get_value(bml_meta_all,name);
        printf("%3d: %20s\n",i,name);
        if(G_VALUE_TYPE(value)==GST_TYPE_STRUCTURE) {
          GstStructure *bml_meta=g_value_get_boxed(value);
          gint j,m=gst_structure_n_fields(bml_meta);
          GQuark bml_type=gst_structure_get_name_id(bml_meta);
          
          if(bml_type==bmln_type) {
            res&=bmln_gstbml_register_element(plugin,bml_meta);
          }
#if HAVE_BMLW
          else {
            res&=bmlw_gstbml_register_element(plugin,bml_meta);
          }
#endif
  
          printf("  %3d entries\n",m);
          for(j=0;j<m;j++) {
            name=gst_structure_nth_field_name(bml_meta,j); 
            value=gst_structure_get_value(bml_meta,name);
            desc=g_strdup_value_contents(value);
            printf("  %3d: %20s: %s; %s\n",j,name,G_VALUE_TYPE_NAME(value),desc);
            g_free(desc);
          }
        }
      }
    }
  }
#endif

  /* @todo: we need to reconsider this when BUILD_STRUCTURE is done
  g_hash_table_destroy(bml_category_by_machine_name);
  g_hash_table_destroy(bml_preset_path_by_descriptor);
  g_hash_table_destroy(bml_help_uri_by_descriptor);
  g_hash_table_destroy(bml_descriptors_by_element_type);
  */
  /* @todo: we can't release this (yet)
  g_hash_table_destroy(bml_descriptors_by_voice_type);
  */
  return(res);
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bml",
    "buzz machine loader - all buzz machines",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)
