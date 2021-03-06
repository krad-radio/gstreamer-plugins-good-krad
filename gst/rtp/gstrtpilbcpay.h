/* GStreamer
 * Copyright (C) <2006> Philippe Khalaf <burger@speedy.org> 
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

#ifndef __GST_RTP_ILBC_PAY_H__
#define __GST_RTP_ILBC_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstbasertpaudiopayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_ILBC_PAY \
  (gst_rtp_ilbc_pay_get_type())
#define GST_RTP_ILBC_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_ILBC_PAY,GstRTPILBCPay))
#define GST_RTP_ILBC_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_ILBC_PAY,GstRTPILBCPayClass))
#define GST_IS_RTP_ILBC_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_ILBC_PAY))
#define GST_IS_RTP_ILBC_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_ILBC_PAY))

typedef struct _GstRTPILBCPay GstRTPILBCPay;
typedef struct _GstRTPILBCPayClass GstRTPILBCPayClass;

struct _GstRTPILBCPay
{
  GstBaseRTPAudioPayload audiopayload;

  gint mode;
};

struct _GstRTPILBCPayClass
{
  GstBaseRTPAudioPayloadClass parent_class;
};

GType gst_rtp_ilbc_pay_get_type (void);

gboolean gst_rtp_ilbc_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_ILBC_PAY_H__ */
