/* $Id: e-gst-note2frequency.c,v 1.1 2006-01-07 16:50:05 ensonic Exp $ */

#include "m-gst-buzztard.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  gst_buzztard_setup();
  //GST_INFO("================================================================================");
}

static void test_teardown(void) {
  gst_buzztard_teardown();
}

//-- tests

START_TEST(test_create_obj) {
  GstNote2Frequency *n2f;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  fail_unless(G_OBJECT(n2f)->ref_count == 1, NULL);
  // free application
  g_object_checked_unref(n2f);
}
END_TEST


TCase *gst_buzztard_note2frequency_example_case(void) {
  TCase *tc = tcase_create("GstNote2FrequencyExamples");

  tcase_add_test(tc,test_create_obj);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
