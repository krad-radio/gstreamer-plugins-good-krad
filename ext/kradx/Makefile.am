plugin_LTLIBRARIES = libgstkradx.la

libgstkradx_la_SOURCES = gstkradx.c 
libgstkradx_la_CFLAGS = $(GST_BASE_CFLAGS) $(GST_CFLAGS) $(KRADX_CFLAGS)
libgstkradx_la_LIBADD = $(GST_BASE_LIBS) $(GST_LIBS) $(KRADX_LIBS)
libgstkradx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstkradx_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstkradx.h
