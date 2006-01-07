/* $Id: t-gst-note2frequency.c,v 1.1 2006-01-07 17:33:28 ensonic Exp $ */

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

START_TEST(test_translate_null) {
  GstNote2Frequency *n2f;
  gdouble frq;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)&~G_LOG_LEVEL_CRITICAL);
  
  frq=gst_note_2_frequency_translate(n2f,NULL);
  fail_unless(frq == 0.0, NULL);

  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_length) {
  GstNote2Frequency *n2f;
  gdouble frq;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)&~G_LOG_LEVEL_CRITICAL);

  frq=gst_note_2_frequency_translate(n2f,"x");
  fail_unless(frq == 0.0, NULL);

  frq=gst_note_2_frequency_translate(n2f,"x-");
  fail_unless(frq == 0.0, NULL);

  frq=gst_note_2_frequency_translate(n2f,"x-000");
  fail_unless(frq == 0.0, NULL);

  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_delim) {
  GstNote2Frequency *n2f;
  gdouble frq;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)&~G_LOG_LEVEL_CRITICAL);

  frq=gst_note_2_frequency_translate(n2f,"C+3");
  fail_unless(frq == 0.0, NULL);

  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref(n2f);
}
END_TEST


TCase *gst_buzztard_note2frequency_test_case(void) {
  TCase *tc = tcase_create("GstNote2FrequencyTests");

  tcase_add_test(tc,test_translate_null);
  tcase_add_test(tc,test_translate_length);
  tcase_add_test(tc,test_translate_delim);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
