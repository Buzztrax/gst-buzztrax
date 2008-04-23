/* $Id$
 *
 * Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef __BT_SAMPLER_H__
#define __BT_SAMPLER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define BT_TYPE_SAMPLER               (bt_sampler_get_type())
#define BT_SAMPLER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SAMPLER, BtSampler))
#define BT_IS_SAMPLER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SAMPLER))
#define BT_SAMPLER_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BT_TYPE_SAMPLER, BtSamplerInterface))


typedef struct _BtSampler BtSampler; /* dummy object */
typedef struct _BtSamplerInterface BtSamplerInterface;

struct _BtSamplerInterface
{
  GTypeInterface parent;
};

GType bt_sampler_get_type(void);

G_END_DECLS

#endif /* __BT_SAMPLER_H__ */
