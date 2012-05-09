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
#include <JavaScriptCore/JavaScript.h>
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
update_frame_height (WebKitWebView *webview,
		     WebKitWebFrame *frame)
{
	/* It's an iframe for a content id, get height */
	JSGlobalContextRef frame_context;
	JSStringRef script_str;
	JSValueRef result;

	frame_context = webkit_web_frame_get_global_context (frame);
	script_str = JSStringCreateWithUTF8CString ("document.documentElement.scrollHeight;");
	result = JSEvaluateScript (frame_context, script_str, NULL, NULL, 0, NULL);
	JSStringRelease (script_str);

	if (JSValueIsNumber (frame_context, result)) {
		JSGlobalContextRef main_context;
		double height = JSValueToNumber (frame_context, result, NULL);
		char *update_script;
		JSStringRef update_script_str;

		main_context = webkit_web_frame_get_global_context (webkit_web_view_get_main_frame (webview));
		update_script = g_strdup_printf ("updateContentIdFrame ('%s', %f);",
						 webkit_web_frame_get_name (frame),
						 height);
		update_script_str = JSStringCreateWithUTF8CString (update_script);
		JSEvaluateScript (main_context, update_script_str, NULL, NULL, 0, NULL);
		JSStringRelease (update_script_str);
		g_free (update_script);
	}
}

static void
on_document_load_finished (WebKitWebView *webview,
			   WebKitWebFrame *frame,
			   ImWindow *window)
{
	SoupURI *uri;

	uri = soup_uri_new (webkit_web_frame_get_uri (frame));
	if (g_strcmp0 (uri->scheme, "cid") == 0) {
		update_frame_height (webview, frame);
	}
	soup_uri_free (uri);
}

static gboolean
on_mime_type_policy_decision_requested (WebKitWebView *web_view,
					WebKitWebFrame *frame,
					WebKitNetworkRequest *request,
					const char *mimetype,
					WebKitWebPolicyDecision *policy_decision,
					gpointer user_data)
{
	gboolean handled = FALSE;
	SoupURI *uri;

	uri = soup_uri_new (webkit_network_request_get_uri (request));
	if (g_strcmp0 (uri->scheme, "cid") != 0) {
		if (!webkit_web_view_can_show_mime_type (web_view, mimetype)) {
			webkit_web_policy_decision_download (policy_decision);
			handled = TRUE;
		}
	}

	soup_uri_free (uri);

	return handled;
}

static gboolean
on_navigation_policy_decision_requested (WebKitWebView *web_view,
					 WebKitWebFrame *frame,
					 WebKitNetworkRequest *request,
					 WebKitWebNavigationAction *action,
					 WebKitWebPolicyDecision *policy_decision,
					 gpointer user_data)
{
	gboolean handled = FALSE;
	SoupURI *uri;

	uri = soup_uri_new (webkit_network_request_get_uri (request));
	if (webkit_web_navigation_action_get_reason (action) == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED &&
	    g_strcmp0 (uri->scheme, "cid") == 0) {
		const char *query;

		query = soup_uri_get_query (uri);
		if (query) {
			GHashTable *params;
			params = soup_form_decode (query);

			if ((!g_strcmp0 (g_hash_table_lookup (params, "mode"), "save") == 0) ||
			    (!g_strcmp0 (g_hash_table_lookup (params, "mode"), "open") == 0)) {
				webkit_web_policy_decision_download (policy_decision);
				handled = TRUE;
			}
			g_hash_table_destroy (params);
		}
	}

	soup_uri_free (uri);

	return handled;
}

static gboolean
on_download_requested (WebKitWebView*web_view,
		       GObject *download,
		       gpointer user_data)
{
	SoupURI *uri;
	const char *query;

	uri = soup_uri_new (webkit_download_get_uri (WEBKIT_DOWNLOAD (download)));
	query = soup_uri_get_query (uri);

	if (query) {
		GHashTable *params;
		params = soup_form_decode (query);
		
		if (g_strcmp0 (g_hash_table_lookup (params, "mode"), "save") == 0) {
			GtkWidget *dialog;

			dialog = gtk_file_chooser_dialog_new ("Save attachment...", GTK_WINDOW (user_data),
							      GTK_FILE_CHOOSER_ACTION_SAVE,
							      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
							      NULL);

			gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD));
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), webkit_download_get_suggested_filename (WEBKIT_DOWNLOAD (download)));
			if (gtk_dialog_run (GTK_DIALOG (dialog)) ==  GTK_RESPONSE_ACCEPT) {
				webkit_download_set_destination_uri (WEBKIT_DOWNLOAD (download), gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog)));
			}
			gtk_widget_destroy (dialog);
		} else if (g_strcmp0 (g_hash_table_lookup (params, "mode"), "open") == 0) {
			g_warning ("Opening to tmp still not implemented");
		}
		g_hash_table_destroy (params);
	}

	soup_uri_free (uri);
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
		"enable-frame-flattening", TRUE,
		NULL);
  gtk_widget_show (priv->webview);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->webview);

  main_view_uri = g_filename_to_uri (HTMLDIR "main-view.html", NULL, NULL);
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (priv->webview),
			    main_view_uri);
  g_signal_connect (G_OBJECT (priv->webview), "document-load-finished",
		    G_CALLBACK (on_document_load_finished), window);
  g_signal_connect (G_OBJECT (priv->webview), "mime-type-policy-decision-requested",
		    G_CALLBACK (on_mime_type_policy_decision_requested), window);
  g_signal_connect (G_OBJECT (priv->webview), "navigation-policy-decision-requested",
		    G_CALLBACK (on_navigation_policy_decision_requested), window);
  g_signal_connect (G_OBJECT (priv->webview), "download-requested",
		    G_CALLBACK (on_download_requested), window);
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
