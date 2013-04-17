/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "glib.h"
#include "gst/gst.h"

#define GST_CAT_DEFAULT gst_buzztard_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

extern Suite *gst_buzztard_note2frequency_suite (void);

gint test_argc = 1;
gchar test_arg0[] = "check_gst_buzzard";
gchar *test_argv[1];
gchar **test_argvptr;

static void
check_log_handler (const gchar * const log_domain,
    const GLogLevelFlags log_level, const gchar * const message,
    gpointer const user_data)
{
  GstDebugLevel level;

  //-- format  
  switch (log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_LEVEL_ERROR:
      level = GST_LEVEL_ERROR;
      break;
    case G_LOG_LEVEL_CRITICAL:
      level = GST_LEVEL_WARNING;
      break;
    case G_LOG_LEVEL_WARNING:
      level = GST_LEVEL_WARNING;
      break;
    case G_LOG_LEVEL_MESSAGE:
      level = GST_LEVEL_INFO;
      break;
    case G_LOG_LEVEL_INFO:
      level = GST_LEVEL_INFO;
      break;
    case G_LOG_LEVEL_DEBUG:
      level = GST_LEVEL_DEBUG;
      break;
    default:
      level = GST_LEVEL_LOG;
      break;
  }
  gst_debug_log (GST_CAT_DEFAULT, level, "?", "?", 0, NULL, "%s", message);
}

/* common setup and teardown code */
void
gst_buzztard_setup (void)
{
  gst_init (&test_argc, &test_argvptr);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-check", 0,
      "music production environment / unit tests");
  // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name ("GST_*", GST_LEVEL_WARNING);
  gst_debug_category_set_threshold (gst_buzztard_debug, GST_LEVEL_DEBUG);
  // no ansi color codes in logfiles please
  gst_debug_set_colored (FALSE);
}

void
gst_buzztard_teardown (void)
{
}

/* start the test run */
int
main (int argc, char **argv)
{
  int nf;
  SRunner *sr;

  g_type_init ();
  //setup_log(argc,argv);
  //setup_log_capture();
  test_argv[0] = test_arg0;
  test_argvptr = test_argv;

  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
      G_LOG_LEVEL_CRITICAL);
  (void) g_log_set_default_handler (check_log_handler, NULL);

  sr = srunner_create (gst_buzztard_note2frequency_suite ());
  // this make tracing errors with gdb easier
  //srunner_set_fork_status(sr,CK_NOFORK);
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
