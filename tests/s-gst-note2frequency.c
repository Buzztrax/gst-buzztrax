/* $Id$ */

#include "m-gst-buzztard.h"

extern TCase *gst_buzztard_note2frequency_test_case(void);
extern TCase *gst_buzztard_note2frequency_example_case(void);

Suite *gst_buzztard_note2frequency_suite(void) { 
  Suite *s=suite_create("GstNote2Frequency"); 

  suite_add_tcase(s,gst_buzztard_note2frequency_test_case());
  suite_add_tcase(s,gst_buzztard_note2frequency_example_case());
  return(s);
}
