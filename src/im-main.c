/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-main.c : Runtime startup */

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

#include <config.h>

#include <im-window.h>
#include <im-service-mgr.h>
#include <im-soup-request.h>

#include <camel/camel.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-requester.h>
#include <webkit/webkit.h>

static void
on_app_activate (GApplication *app,
		 gpointer userdata)
{
  GList *windows;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));

  if (windows) {
    gtk_window_present (GTK_WINDOW (windows->data));
  } else {
    GtkWidget *main_window;

    main_window = im_window_new (GTK_APPLICATION (app));
    gtk_application_add_window (GTK_APPLICATION (app), GTK_WINDOW (main_window));
    gtk_widget_show (main_window);
  }
}

int main (int argc, char **argv)
{
  GtkApplication *app;
  gint status;
  SoupSessionFeature *requester;

  app = gtk_application_new ("com.igalia.IwkMail", G_APPLICATION_FLAGS_NONE);
  g_object_set (G_OBJECT (app),
		"inactivity-timeout", 30000,
		"register-session", TRUE,
		NULL);
  g_signal_connect (app, "activate",
		    G_CALLBACK (on_app_activate), NULL);

  requester = SOUP_SESSION_FEATURE (soup_requester_new ());
  soup_session_feature_add_feature (requester, IM_TYPE_SOUP_REQUEST);

  soup_session_add_feature (webkit_get_default_session (),
			    requester);
  g_object_unref (requester);

  camel_init (im_service_mgr_get_user_data_dir (), TRUE);
  camel_provider_init ();
  im_service_mgr_get_instance ();

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);
  return status;
}
