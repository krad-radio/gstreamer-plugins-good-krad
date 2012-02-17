/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_KRADXSEND_H__
#define __GST_KRADXSEND_H__

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <stddef.h>
#include <inttypes.h>

G_BEGIN_DECLS

/* Definition of structure storing data for this element. */

typedef struct kradx_base64_St kradx_base64_t;

struct kradx_base64_St {
	int len;
	int chunk;
	char *out;
	char *result;	
};

typedef struct _GstKradxsend GstKradxsend;
struct _GstKradxsend {
  GstBaseSink parent;

  GstPoll *timer;

  gchar *ip;
  guint port;
  gchar *password;
  gchar *mount;
  
  guint16 format;
  
  gboolean connected;

  int header_pos;
  char headers[1024];
  char auth_base64[512];
  char auth[512];

  char content_type[128];

  struct sockaddr_in serveraddr;
  struct hostent *hostp;

  int sd;
  int sent;
  unsigned long total_bytes_sent;

  gchar *file;
  int test_fd;

};



/* Standard definition defining a class for this element. */
typedef struct _GstKradxsendClass GstKradxsendClass;
struct _GstKradxsendClass {
  GstBaseSinkClass parent_class;

  /* signal callbacks */
  void (*connection_problem) (GstElement *element,guint errno);
};

/* Standard macros for defining types for this element.  */
#define GST_TYPE_KRADXSEND \
  (gst_kradxsend_get_type())
#define GST_KRADXSEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KRADXSEND,GstKradxsend))
#define GST_KRADXSEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KRADXSEND,GstKradxsendClass))
#define GST_IS_KRADXSEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KRADXSEND))
#define GST_IS_KRADXSEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KRADXSEND))

/* Standard function returning type information. */
GType gst_kradxsend_get_type(void);


G_END_DECLS

#endif /* __GST_KRADXSEND_H__ */
