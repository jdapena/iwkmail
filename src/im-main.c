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
