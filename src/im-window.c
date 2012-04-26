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
