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

#include "m-gst-buzztrax.h"

//-- globals

//-- fixtures

static void
suite_setup (void)
{
  gst_buzztrax_setup ();
  gst_debug_remove_log_function (gst_debug_log_default);
}

static void
suite_teardown (void)
{
  gst_buzztrax_teardown ();
}

//-- tests

START_TEST (test_translate_str_null)
{
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  fail_unless (n2f != NULL, NULL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) &
      ~G_LOG_LEVEL_CRITICAL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, NULL);
  fail_unless (frq == 0.0, NULL);

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
      G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref (n2f);
}

END_TEST
START_TEST (test_translate_str_length)
{
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  fail_unless (n2f != NULL, NULL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) &
      ~G_LOG_LEVEL_CRITICAL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "x");
  fail_unless (frq == 0.0, NULL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "x-");
  fail_unless (frq == 0.0, NULL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "x-000");
  fail_unless (frq == 0.0, NULL);

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
      G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref (n2f);
}

END_TEST
START_TEST (test_translate_str_delim)
{
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  fail_unless (n2f != NULL, NULL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) &
      ~G_LOG_LEVEL_CRITICAL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "C+3");
  fail_unless (frq == 0.0, NULL);

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
      G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref (n2f);
}

END_TEST
START_TEST (test_translate_enum_range)
{
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  fail_unless (n2f != NULL, NULL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) &
      ~G_LOG_LEVEL_CRITICAL);

  frq = gstbt_tone_conversion_translate_from_number (n2f, 1 + (16 * 10));
  fail_unless (frq == 0.0, NULL);

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
      G_LOG_LEVEL_CRITICAL);
  // free object
  g_object_checked_unref (n2f);
}

END_TEST

TCase *
gst_buzztrax_note2frequency_test_case (void)
{
  TCase *tc = tcase_create ("GstBtToneConversionTests");

  tcase_add_test (tc, test_translate_str_null);
  tcase_add_test (tc, test_translate_str_length);
  tcase_add_test (tc, test_translate_str_delim);
  tcase_add_test (tc, test_translate_enum_range);
  tcase_add_unchecked_fixture (tc, suite_setup, suite_teardown);
  return (tc);
}
