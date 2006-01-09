/* $Id: e-gst-note2frequency.c,v 1.3 2006-01-09 20:13:42 ensonic Exp $ */

#include "m-gst-buzztard.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  gst_buzztard_setup();
  GST_INFO("================================================================================");
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

  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_str_base) {
  GstNote2Frequency *n2f;
  gdouble frq;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);

  frq=gst_note_2_frequency_translate_from_string(n2f,"A-3");
  //g_print("frq=%lf\n",frq);
  fail_unless(frq==440.0, NULL);

  frq=gst_note_2_frequency_translate_from_string(n2f,"a-3");
  //g_print("frq=%lf\n",frq);
  fail_unless(frq==440.0, NULL);
  
  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_str_series) {
  GstNote2Frequency *n2f;
  gdouble frq,frq_prev=0.0;
  gchar *notes[]={
	"c-0","c#0","d-0","d#0","e-0","f-0","f#0","g-0","g#0","a-0","a#0","h-0",
	"c-1","c#1","d-1","d#1","e-1","f-1","f#1","g-1","g#1","a-1","a#1","h-1",
	"c-2",NULL };
  guint i=0;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  
  while(notes[i]) {
	frq=gst_note_2_frequency_translate_from_string(n2f,notes[i]);
	//g_print("%s -> frq=%lf\n",notes[i],frq);
	fail_unless(frq!=0.0, NULL);
	fail_unless(frq>frq_prev, NULL);
	frq_prev=frq;
	i++;
  }
  
  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_num_base) {
  GstNote2Frequency *n2f;
  gdouble frq;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);

  frq=gst_note_2_frequency_translate_from_number(n2f,12*3+9);
  //g_print("frq=%lf\n",frq);
  fail_unless(frq==440.0, NULL);
  
  // free object
  g_object_checked_unref(n2f);
}
END_TEST

START_TEST(test_translate_num_series) {
  GstNote2Frequency *n2f;
  gdouble frq,frq_prev=0.0;
  guint i=0;
  
  n2f=gst_note_2_frequency_new(GST_NOTE_2_FREQUENCY_CROMATIC);
  fail_unless(n2f != NULL, NULL);
  
  for(i=0;i<12*10;i++) {
	frq=gst_note_2_frequency_translate_from_number(n2f,i);
	//g_print("%3u -> frq=%lf\n",i,frq);
	fail_unless(frq!=0.0, NULL);
	fail_unless(frq>frq_prev, NULL);
	frq_prev=frq;
  }
  
  // free object
  g_object_checked_unref(n2f);
}
END_TEST


TCase *gst_buzztard_note2frequency_example_case(void) {
  TCase *tc = tcase_create("GstNote2FrequencyExamples");

  tcase_add_test(tc,test_create_obj);
  tcase_add_test(tc,test_translate_str_base);
  tcase_add_test(tc,test_translate_str_series);
  tcase_add_test(tc,test_translate_num_base);
  tcase_add_test(tc,test_translate_num_series);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
