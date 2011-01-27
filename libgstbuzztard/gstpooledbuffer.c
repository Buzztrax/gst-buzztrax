/* GStreamer
 * Copyright (C) 2010 Stefan Kost <ensonic@users.sf.net>
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

/**
 * SECTION:gstbufferpool
 * @short_description: baseclass reusable buffers
 *
 * This is a baseclass for implementing buffers that are reused through a.
 * #GstBufferPool. The class mostly adds a pool field that points from the
 * buffer to the pool that owns it and a finalize implementation that resurrects
 * buffers and re-enqueues them into their pool.
 */

#include "gstpooledbuffer.h"
#include "gstbufferpool.h"

GST_DEBUG_CATEGORY_STATIC (gst_pooled_buffer_debug);
#define GST_CAT_DEFAULT gst_pooled_buffer_debug

static void gst_pooled_buffer_finalize (GstPooledBuffer * object);

G_DEFINE_TYPE_WITH_CODE (GstPooledBuffer, gst_pooled_buffer, GST_TYPE_BUFFER,
    GST_DEBUG_CATEGORY_INIT (gst_pooled_buffer_debug, "pooled-buffer", 0,
        "poolable buffers"));

static void
gst_pooled_buffer_class_init (GstPooledBufferClass * klass)
{
  GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS (klass);

  mini_object_class->finalize = (GstMiniObjectFinalizeFunction)
      gst_pooled_buffer_finalize;
}

static void
gst_pooled_buffer_init (GstPooledBuffer * self)
{
  GST_WARNING ("init pooled buffer %p", self);
}

static void
gst_pooled_buffer_finalize (GstPooledBuffer * self)
{
  GstBufferPool *pool = self->pool;

  GST_WARNING ("finalizing pooled buffer %p", self);

  if (pool) {
    GST_WARNING ("reviving pooled buffer %p", self);
    gst_buffer_ref ((GstBuffer *)self);
    gst_buffer_pool_release_buffer (pool, (GstBuffer *)self);
  } else {
    GST_MINI_OBJECT_CLASS (gst_pooled_buffer_parent_class)->finalize
        (GST_MINI_OBJECT (self));
  }
}

GstBuffer *
gst_pooled_buffer_new (GstBufferPool *pool)
{
  GstPooledBuffer *buf;

  buf = (GstPooledBuffer *) gst_mini_object_new (GST_TYPE_POOLED_BUFFER);
  buf->pool = pool;
  return (GstBuffer *)buf;
}
