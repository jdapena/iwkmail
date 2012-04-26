/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-window.c : Application window */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "im-window.h"

#include <gtk/gtk.h>
#include <webkit/webkit.h>

G_DEFINE_TYPE (ImWindow, im_window, GTK_TYPE_WINDOW);
#define IM_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), IM_TYPE_WINDOW, ImWindowPrivate))

struct _ImWindowPrivate
{
  GtkWidget *webview;
};

static gboolean
on_delete_event (GtkWidget *widget,
		 GdkEvent *event,
		 gpointer userdata)
{
  g_application_quit (g_application_get_default ());
  return TRUE;
}

static void
im_window_dispose (GObject *object)
{
}

static void
im_window_finalize (GObject *object)
{
}

static void
im_window_class_init (ImWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->dispose = im_window_dispose;
  object_class->finalize = im_window_finalize;

  g_type_class_add_private (object_class, sizeof (ImWindowPrivate));
}

static void
im_window_init (ImWindow *window)
{
  ImWindowPrivate *priv;
  GtkWidget *scrolled_window;
  char *main_view_uri;
  WebKitWebSettings *settings;

  window->priv = IM_WINDOW_GET_PRIVATE (window);
  priv = window->priv;

  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_window);
  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  priv->webview = webkit_web_view_new ();

  settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->webview));
  g_object_set (G_OBJECT (settings),
		"enable-file-access-from-file-uris", TRUE,
		NULL);
  gtk_widget_show (priv->webview);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->webview);

  main_view_uri = g_filename_to_uri (HTMLDIR "main-view.html", NULL, NULL);
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (priv->webview),
			    main_view_uri);
  g_free (main_view_uri);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (on_delete_event), window);
}

/**
 * im_window_new:
 *
 * Equivalent to g_object_new() but returns an #ImWindow so you don't have
 * to cast it.
 *
 * Return value: a new #ImWindow
 **/
GtkWidget *
im_window_new (GtkApplication *application)
{
	return GTK_WIDGET (g_object_new (IM_TYPE_WINDOW,
					 NULL));
}
