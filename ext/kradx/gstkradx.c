/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2006> Tim-Philipp Müller <tim centricular net>
 * Copyright (C) <2012> Ralph Giles <giles@mozilla.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstkradx.h"
#include <stdlib.h>
#include <string.h>

#include "gst/gst-i18n-plugin.h"

GST_DEBUG_CATEGORY_STATIC (kradx_debug);
#define GST_CAT_DEFAULT kradx_debug

enum
{
  SIGNAL_CONNECTION_PROBLEM,    /* 0.11 FIXME: remove this */
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_IP,                       /* the ip of the server */
  ARG_PORT,                     /* the encoder port number on the server */
  ARG_PASSWORD,                 /* the encoder password on the server */
  ARG_MOUNT,                    /* mountpoint of stream (icecast only) */
  ARG_FILE,                     /* also write to a file for debuging */
};

#define DEFAULT_IP           "127.0.0.1"
#define DEFAULT_PORT         8000
#define DEFAULT_PASSWORD     "hackme"
#define DEFAULT_MOUNT        ""

static GstElementClass *parent_class = NULL;

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/ogg; "
        "audio/mpeg, mpegversion = (int) 1, layer = (int) [ 1, 3 ]" "; video/webm"));

static void gst_kradxsend_class_init (GstKradxsendClass * klass);
static void gst_kradxsend_base_init (GstKradxsendClass * klass);
static void gst_kradxsend_init (GstKradxsend * kradxsend);
static void gst_kradxsend_finalize (GstKradxsend * kradxsend);

static gboolean gst_kradxsend_event (GstBaseSink * sink, GstEvent * event);
static gboolean gst_kradxsend_unlock (GstBaseSink * basesink);
static gboolean gst_kradxsend_unlock_stop (GstBaseSink * basesink);
static GstFlowReturn gst_kradxsend_render (GstBaseSink * sink,
    GstBuffer * buffer);
static gboolean gst_kradxsend_start (GstBaseSink * basesink);
static gboolean gst_kradxsend_stop (GstBaseSink * basesink);

static void gst_kradxsend_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_kradxsend_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_kradxsend_setcaps (GstPad * pad, GstCaps * caps);

static guint gst_kradxsend_signals[LAST_SIGNAL] = { 0 };

static void base64_encode(char *dest, char *src) {

	kradx_base64_t *base64 = calloc(1, sizeof(kradx_base64_t));

	static char base64table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
									'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
									'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', 
									'5', '6', '7', '8', '9', '+', '/' };

	base64->len = strlen(src);
	base64->out = (char *) malloc(base64->len * 4 / 3 + 4);
	base64->result = base64->out;

	while(base64->len > 0) {
		base64->chunk = (base64->len > 3) ? 3 : base64->len;
		*base64->out++ = base64table[(*src & 0xFC) >> 2];
		*base64->out++ = base64table[((*src & 0x03) << 4) | ((*(src + 1) & 0xF0) >> 4)];
		switch(base64->chunk) {
			case 3:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2) | ((*(src + 2) & 0xC0) >> 6)];
				*base64->out++ = base64table[(*(src + 2)) & 0x3F];
				break;

			case 2:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2)];
				*base64->out++ = '=';
				break;

			case 1:
				*base64->out++ = '=';
				*base64->out++ = '=';
				break;
		}

		src += base64->chunk;
		base64->len -= base64->chunk;
	}

	*base64->out = 0;

	strcpy(dest, base64->result);
	free(base64->result);
	free(base64);

}

GType
gst_kradxsend_get_type (void)
{
  static GType kradxsend_type = 0;

  if (!kradxsend_type) {
    static const GTypeInfo kradxsend_info = {
      sizeof (GstKradxsendClass),
      (GBaseInitFunc) gst_kradxsend_base_init,
      NULL,
      (GClassInitFunc) gst_kradxsend_class_init,
      NULL,
      NULL,
      sizeof (GstKradxsend),
      0,
      (GInstanceInitFunc) gst_kradxsend_init,
    };

    static const GInterfaceInfo tag_setter_info = {
      NULL,
      NULL,
      NULL
    };

    kradxsend_type =
        g_type_register_static (GST_TYPE_BASE_SINK, "GstKradxsend",
        &kradxsend_info, 0);

    g_type_add_interface_static (kradxsend_type, GST_TYPE_TAG_SETTER,
        &tag_setter_info);

  }
  return kradxsend_type;
}

static void
gst_kradxsend_base_init (GstKradxsendClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_set_details_simple (element_class, "Icecast / KXPDR network sink",
      "Sink/Network", "Sends data to an icecast server",
      "David Richards <kradradio@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (kradx_debug, "kradx", 0, "kradxsend element");
}

static void
gst_kradxsend_class_init (GstKradxsendClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_kradxsend_set_property;
  gobject_class->get_property = gst_kradxsend_get_property;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_kradxsend_finalize;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_IP,
      g_param_spec_string ("ip", "ip", "ip", DEFAULT_IP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_PORT,
      g_param_spec_int ("port", "port", "port", 1, G_MAXUSHORT, DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_PASSWORD,
      g_param_spec_string ("password", "password", "password", DEFAULT_PASSWORD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MOUNT,
      g_param_spec_string ("mount", "mount", "mount", DEFAULT_MOUNT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_FILE,
      g_param_spec_string ("file", "file", "file", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* signals */
  gst_kradxsend_signals[SIGNAL_CONNECTION_PROBLEM] =
      g_signal_new ("connection-problem", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_CLEANUP, G_STRUCT_OFFSET (GstKradxsendClass,
          connection_problem), NULL, NULL, g_cclosure_marshal_VOID__INT,
      G_TYPE_NONE, 1, G_TYPE_INT);

  gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_kradxsend_start);
  gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_kradxsend_stop);
  gstbasesink_class->unlock = GST_DEBUG_FUNCPTR (gst_kradxsend_unlock);
  gstbasesink_class->unlock_stop =
      GST_DEBUG_FUNCPTR (gst_kradxsend_unlock_stop);
  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_kradxsend_render);
  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_kradxsend_event);
}

static void
gst_kradxsend_init (GstKradxsend * kradxsend)
{
  gst_base_sink_set_sync (GST_BASE_SINK (kradxsend), FALSE);

  gst_pad_set_setcaps_function (GST_BASE_SINK_PAD (kradxsend),
      GST_DEBUG_FUNCPTR (gst_kradxsend_setcaps));

  kradxsend->timer = gst_poll_new_timer ();

  kradxsend->ip = g_strdup (DEFAULT_IP);
  kradxsend->port = DEFAULT_PORT;
  kradxsend->password = g_strdup (DEFAULT_PASSWORD);
  kradxsend->mount = g_strdup (DEFAULT_MOUNT);
  kradxsend->file = NULL;
  kradxsend->test_fd = 0;
}

static void
gst_kradxsend_finalize (GstKradxsend * kradxsend)
{
  g_free (kradxsend->ip);
  g_free (kradxsend->password);
  g_free (kradxsend->mount);
  if (kradxsend->file) {
    g_free (kradxsend->file);
    if (kradxsend->test_fd) {
	    close (kradxsend->test_fd);
	}
  }
  gst_poll_free (kradxsend->timer);
  
  close (kradxsend->sd);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (kradxsend));
}

static gboolean
gst_kradxsend_event (GstBaseSink * sink, GstEvent * event)
{
  GstKradxsend *kradxsend;
  gboolean ret = TRUE;

  kradxsend = GST_KRADXSEND (sink);

  GST_LOG_OBJECT (kradxsend, "got %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    default:{
      GST_LOG_OBJECT (kradxsend, "let base class handle event");
      if (GST_BASE_SINK_CLASS (parent_class)->event) {
        event = gst_event_ref (event);
        ret = GST_BASE_SINK_CLASS (parent_class)->event (sink, event);
      }
      break;
    }
  }

  return ret;
}

static gboolean
gst_kradxsend_start (GstBaseSink * basesink)
{
  GstKradxsend *sink = GST_KRADXSEND (basesink);

  GST_DEBUG_OBJECT (sink, "starting");

  return TRUE;


}

static gboolean
gst_kradxsend_connect (GstKradxsend * sink)
{

	int sret;

	GST_DEBUG_OBJECT (sink, "KradX Source: Connecting to %s:%d\n", sink->ip, sink->port);

	if ((sink->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		GST_DEBUG_OBJECT (sink, "Krad EBML Source: Socket Error\n");
		exit(1);
	}

	memset(&sink->serveraddr, 0x00, sizeof(struct sockaddr_in));
	sink->serveraddr.sin_family = AF_INET;
	sink->serveraddr.sin_port = htons(sink->port);
	
	if ((sink->serveraddr.sin_addr.s_addr = inet_addr(sink->ip)) == (unsigned long)INADDR_NONE)
	{
		// get host address 
		sink->hostp = gethostbyname(sink->ip);
		if(sink->hostp == (struct hostent *)NULL)
		{
			GST_DEBUG_OBJECT (sink, "KradX Source: Mount problem\n");
			close(sink->sd);
			exit(1);
		}
		memcpy(&sink->serveraddr.sin_addr, sink->hostp->h_addr, sizeof(sink->serveraddr.sin_addr));
	}

	// connect() to server. 
	if((sink->sent = connect(sink->sd, (struct sockaddr *)&sink->serveraddr, sizeof(sink->serveraddr))) < 0)
	{
		GST_DEBUG_OBJECT (sink, "KradX Source: Connect Error\n");
		exit(1);
	} else {
	
		sink->connected = 1;
		GST_DEBUG_OBJECT (sink, "KradX Source: Connected.\n");
	}
	
	strcpy(sink->content_type, "video/webm");
	//strcpy(krad_mkvsource->content_type, "video/x-matroska");
	//strcpy(krad_mkvsource->content_type, "audio/mpeg");
	//strcpy(krad_mkvsource->content_type, "application/ogg");
	sprintf( sink->auth, "source:%s", sink->password );
	base64_encode( sink->auth_base64, sink->auth );
	sink->header_pos = sprintf( sink->headers, "SOURCE /%s ICE/1.0\r\n", sink->mount);
	sink->header_pos += sprintf( sink->headers + sink->header_pos, "content-type: %s\r\n", sink->content_type);
	sink->header_pos += sprintf( sink->headers + sink->header_pos, "Authorization: Basic %s\r\n", sink->auth_base64);
	sink->header_pos += sprintf( sink->headers + sink->header_pos, "\r\n");
	
	sret = send(sink->sd, sink->headers, strlen(sink->headers), 0);
	
		GST_DEBUG_OBJECT (sink, "KradX Source: Sent Headers. %d %d %s\n", sret, strlen(sink->headers), sink->headers);

  GST_DEBUG_OBJECT (sink, "connected to server");
  sink->connected = TRUE;

  return TRUE;

}

static gboolean
gst_kradxsend_stop (GstBaseSink * basesink)
{
  GstKradxsend *sink = GST_KRADXSEND (basesink);

  if (sink->connected) {
   close (sink->sd);
   sink->sd = 0;
  }
  
  if (sink->test_fd) {
    close (sink->test_fd);
  }

  sink->connected = FALSE;

  return TRUE;
}

static gboolean
gst_kradxsend_unlock (GstBaseSink * basesink)
{
  GstKradxsend *sink;

  sink = GST_KRADXSEND (basesink);

  GST_DEBUG_OBJECT (basesink, "unlock");
  gst_poll_set_flushing (sink->timer, TRUE);

  return TRUE;
}

static gboolean
gst_kradxsend_unlock_stop (GstBaseSink * basesink)
{
  GstKradxsend *sink;

  sink = GST_KRADXSEND (basesink);

  GST_DEBUG_OBJECT (basesink, "unlock_stop");
  gst_poll_set_flushing (sink->timer, FALSE);

  return TRUE;
}

static GstFlowReturn
gst_kradxsend_render (GstBaseSink * basesink, GstBuffer * buf)
{
  GstKradxsend *sink;
  int sent;
  int sended;
  int fdret;
  sink = GST_KRADXSEND (basesink);

  /* presumably we connect here because we need to know the format before
   * we can set up the connection, which we don't know yet in _start() */
  if (!sink->connected) {
    if (!gst_kradxsend_connect (sink))
      return GST_FLOW_ERROR;
  }

  GST_LOG_OBJECT (sink, "sending %u bytes of data", GST_BUFFER_SIZE (buf));

  sent = 0;	
  sended = 0;
  fdret = 0;
  
  if (sink->test_fd != 0) {
    fdret = write(sink->test_fd, GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));
  
    if (fdret != GST_BUFFER_SIZE (buf)) {
  	  GST_LOG_OBJECT (sink, "debug file problem");
    }
  
  }
  
  while (sent != GST_BUFFER_SIZE (buf)) {

    sended = send ( sink->sd, GST_BUFFER_DATA (buf) + sent, 
					GST_BUFFER_SIZE (buf) - sent, 0);

	if (sended <= 0) {			
		break;
	}
    sent += sended;	
  }

  if (sent != GST_BUFFER_SIZE (buf)) {
	GST_ELEMENT_ERROR (sink, RESOURCE, WRITE, (NULL),
	    ("kradx_send() failed try %d got %d", GST_BUFFER_SIZE (buf), sent));
	g_signal_emit (sink, gst_kradxsend_signals[SIGNAL_CONNECTION_PROBLEM], 0,
	    666);
	return GST_FLOW_ERROR;
  }


  return GST_FLOW_OK;

}

static void
gst_kradxsend_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstKradxsend *kradxsend;

  kradxsend = GST_KRADXSEND (object);
  switch (prop_id) {

    case ARG_IP:
      if (kradxsend->ip)
        g_free (kradxsend->ip);
      kradxsend->ip = g_strdup (g_value_get_string (value));
      break;
    case ARG_PORT:
      kradxsend->port = g_value_get_int (value);
      break;
    case ARG_PASSWORD:
      if (kradxsend->password)
        g_free (kradxsend->password);
      kradxsend->password = g_strdup (g_value_get_string (value));
      break;
    case ARG_MOUNT:            /* mountpoint of stream (icecast only) */
      if (kradxsend->mount)
        g_free (kradxsend->mount);
      kradxsend->mount = g_strdup (g_value_get_string (value));
      break;
    case ARG_FILE:            /* mountpoint of stream (icecast only) */
      if (kradxsend->file)
        g_free (kradxsend->file);
      kradxsend->file = g_strdup (g_value_get_string (value));
      if (strlen(kradxsend->file)) {
        kradxsend->test_fd = open(kradxsend->file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      }
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_kradxsend_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstKradxsend *kradxsend;

  kradxsend = GST_KRADXSEND (object);
  switch (prop_id) {

    case ARG_IP:
      g_value_set_string (value, kradxsend->ip);
      break;
    case ARG_PORT:
      g_value_set_int (value, kradxsend->port);
      break;
    case ARG_PASSWORD:
      g_value_set_string (value, kradxsend->password);
      break;
    case ARG_MOUNT:
      g_value_set_string (value, kradxsend->mount);
      break;
    case ARG_FILE:
      g_value_set_string (value, kradxsend->file);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_kradxsend_setcaps (GstPad * pad, GstCaps * caps)
{
  const gchar *mimetype;
  GstKradxsend *kradxsend;
  gboolean ret = TRUE;

  kradxsend = GST_KRADXSEND (GST_OBJECT_PARENT (pad));

  mimetype = gst_structure_get_name (gst_caps_get_structure (caps, 0));

  GST_DEBUG_OBJECT (kradxsend, "mimetype of caps given is: %s", mimetype);

  if (!strcmp (mimetype, "audio/mpeg")) {
    kradxsend->format = 0;
  } else if (!strcmp (mimetype, "application/ogg")) {
    kradxsend->format = 1;
  } else if (!strcmp (mimetype, "video/webm")) {
    kradxsend->format = 2;
  } else {
    ret = FALSE;
  }

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  return gst_element_register (plugin, "kradxsend", GST_RANK_NONE,
      GST_TYPE_KRADXSEND);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "kradxsend",
    "Sends data to an icecast server using some codes",
    plugin_init,
    VERSION, "LGPL", "libkradx", "http://mozilla.com")
