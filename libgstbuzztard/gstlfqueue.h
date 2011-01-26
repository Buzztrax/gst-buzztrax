/* GStreamer
 * Copyright (C) 2009-2010 Edward Hervey <bilboed@bilboed.com>
 *
 * gstlfqueue.h:
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

#include <glib.h>

#ifndef __GST_LF_QUEUE_H__
#define __GST_LF_QUEUE_H__

typedef struct _GstLFQueue GstLFQueue;

G_BEGIN_DECLS

GstLFQueue *       gst_lf_queue_new         (guint initial_size);
void               gst_lf_queue_free        (GstLFQueue * queue);

void               gst_lf_queue_push        (GstLFQueue* queue, gpointer data);
gpointer           gst_lf_queue_pop         (GstLFQueue* queue);
gpointer           gst_lf_queue_peek        (GstLFQueue* queue);

guint              gst_lf_queue_length      (GstLFQueue * queue);

G_END_DECLS

#endif /* __GST_LF_QUEUE_H__ */
