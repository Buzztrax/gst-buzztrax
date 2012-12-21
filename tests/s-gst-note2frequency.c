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

#include "m-gst-buzztard.h"

extern TCase *gst_buzztard_note2frequency_test_case (void);
extern TCase *gst_buzztard_note2frequency_example_case (void);

Suite *
gst_buzztard_note2frequency_suite (void)
{
  Suite *s = suite_create ("GstBtToneConversion");

  suite_add_tcase (s, gst_buzztard_note2frequency_test_case ());
  suite_add_tcase (s, gst_buzztard_note2frequency_example_case ());
  return (s);
}
