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

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

GstPlugin *bml_plugin;
GHashTable *bml_dllpath_by_element_type;
GHashTable *bml_descriptors_by_element_type;
GHashTable *bml_descriptors_by_voice_type;
GHashTable *bml_help_uri_by_descriptor;
GHashTable *bml_preset_path_by_descriptor;
GType voice_type;

GQuark gst_bml_property_meta_quark_type;

extern gboolean bmlw_describe_plugin(gchar *pathname, gpointer bm);
extern gboolean bmln_describe_plugin(gchar *pathname, gpointer bm);

typedef int (*bsearchcomparefunc)(const void *,const void *);

static int blacklist_compare(const void *node1, const void *node2) {
  //GST_WARNING("comparing '%s' '%s'",node1,*(gchar**)node2);
  return (strcasecmp((gchar *)node1,*(gchar**)node2));
}

/*
 * dir_scan:
 *
 * Search the given directory.
 */
static gboolean dir_scan(const gchar *dir_name) {
  DIR *dir;
  struct dirent *dir_entry;
  gchar *file_name,*entry_name,*ext;
  gpointer bm;
  gboolean res=FALSE,ok;

  const gchar *blacklist[] ={
    "2NDPROCESS NUMBIRD 1.4.DLL",
    "ANEURYSM DISTGARB.DLL",
    "ARGUELLES FX2.DLL",
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
    "JESKOLA REVERB 2.DLL",
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

  dir = opendir(dir_name);
  if (!dir) return(res);

  while(TRUE) {
    dir_entry = readdir (dir);
    if (!dir_entry) {
      closedir(dir);
      return(res);
    }

    entry_name=dir_entry->d_name;
    if(entry_name && (entry_name[0]!='.')) {
      ext=strrchr(entry_name,'.');
      if (ext && (!strcasecmp(ext,".dll") || !strcmp(ext,".so"))) {
        /* test against blacklist */
        if(!bsearch(entry_name, blacklist, G_N_ELEMENTS(blacklist),sizeof(gchar *), blacklist_compare)) {
          file_name=g_build_filename (dir_name, entry_name, NULL);
          GST_INFO("trying plugin '%s','%s'",entry_name,file_name);
          ok=FALSE;
          if(!strcasecmp(ext,".dll")) {
  #if HAVE_BMLW
            if((bm=bmlw_new(file_name))) {
              bmlw_init(bm,0,NULL);
              if(bmlw_describe_plugin(file_name,bm)) {
                /* @todo: free here, or leave on instance open to be used in class init
                 * bmlw_free(bm); */
                res=ok=TRUE;
              }
              else {
                bmlw_free(bm);
              }
            }
  #else
            GST_WARNING("no dll emulation on non x86 platforms");
  #endif
          }
          else {
            if((bm=bmln_new(file_name))) {
              bmln_init(bm,0,NULL);
              if(bmln_describe_plugin(file_name,bm)) {
                /* @todo: free here, or leave on instance open to be used in class init
                 * bmln_free(bm); */
                res=ok=TRUE;
              }
              else {
                bmln_free(bm);
              }
            }
          }
          if(!ok) {
            GST_WARNING("machine %s could not be loaded",entry_name);
            /* @todo: only free here, as we put that into a hashmap in the callback above */
            g_free(file_name);file_name=NULL;
          }
        }
        else {
          GST_WARNING("machine %s is black-listed",entry_name);
        }
      }
    }
  }
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
  gchar *dir;
  const gchar *bml_path;
  const gchar *end_pos, *cur_pos;
  gboolean res=FALSE;

  bml_path = g_getenv("BML_PATH");

  if(!bml_path || !*bml_path) {
#if HAVE_BMLW
    bml_path = LIBDIR G_DIR_SEPARATOR_S "Gear:" LIBDIR G_DIR_SEPARATOR_S "Gear" G_DIR_SEPARATOR_S "Generators:" LIBDIR G_DIR_SEPARATOR_S "Gear" G_DIR_SEPARATOR_S "Effects";
#else
    bml_path = LIBDIR G_DIR_SEPARATOR_S "Gear";
#endif
    GST_WARNING("You do not have a BML_PATH environment variable set, using default: '%s'", bml_path);
  }

  // scan each path component
  cur_pos = bml_path;
  while(*cur_pos != '\0') {
    end_pos = cur_pos;
    while(*end_pos != ':' && *end_pos != '\0') end_pos++;

    if(end_pos > cur_pos) {
      dir=g_strndup(cur_pos,(end_pos-cur_pos));
      res|=dir_scan(dir);
      g_free(dir);
      cur_pos=end_pos;
    }
    if(*cur_pos==':') cur_pos++;
  }
  GST_INFO("after scanning path \"%s\", res=%d",bml_path,res);
  return(res);
}

static gboolean plugin_init (GstPlugin * plugin) {
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bml", GST_DEBUG_FG_GREEN | GST_DEBUG_BG_BLACK | GST_DEBUG_BOLD, "BML");

  GST_INFO("lets go ===========================================================");
  
  oil_init ();

  // init bml library
  if(!bml_setup(0)) {
    GST_WARNING("failed to init bml library");
    return(FALSE);
  }

  // init global data
  // @todo can we make these static again and associate the pointer with the
  // GstPlugin-structure (GstPlugin is not a GObject)?
  bml_dllpath_by_element_type=g_hash_table_new(NULL, NULL);
  bml_descriptors_by_element_type=g_hash_table_new(NULL, NULL);
  bml_descriptors_by_voice_type=g_hash_table_new(NULL, NULL);
  bml_help_uri_by_descriptor=g_hash_table_new(NULL, NULL);
  bml_preset_path_by_descriptor=g_hash_table_new(NULL, NULL);
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
  return(bml_scan());
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
