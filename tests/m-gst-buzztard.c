/* $Id: m-gst-buzztard.c,v 1.2 2006-01-07 17:33:28 ensonic Exp $
 * package unit tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "glib.h"
#include "gst/gst.h"

#define GST_CAT_DEFAULT gst_buzztard_debug
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

extern Suite *gst_buzztard_note2frequency_suite(void);

gint test_argc=1;
gchar test_arg0[]="check_gst_buzzard";
gchar *test_argv[1];
gchar **test_argvptr;

/* common setup and teardown code */
void gst_buzztard_setup(void) {
  gst_init(&test_argc,&test_argvptr);
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-check", 0, "music production environment / unit tests");
  // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_WARNING);
  gst_debug_category_set_threshold(gst_buzztard_debug,GST_LEVEL_DEBUG);
  // no ansi color codes in logfiles please
  gst_debug_set_colored(FALSE);
}

void gst_buzztard_teardown(void) {
}

/* start the test run */
int main(int argc, char **argv) {
  int nf;
  SRunner *sr;

  g_type_init();
  //setup_log(argc,argv);
  //setup_log_capture();
  test_argv[0]=test_arg0;
  test_argvptr=test_argv;
  
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);
  
  sr=srunner_create(gst_buzztard_note2frequency_suite());
  // this make tracing errors with gdb easier
  //srunner_set_fork_status(sr,CK_NOFORK);
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
