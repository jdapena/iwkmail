/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-service-mgr.h : CamelService stores and transports manager */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Alberto Garcia <agarcia@igalia.com>
 *  Felipe Erias Morandeira <femorandeira@igalia.com>
 *  Javier Fernandez Garcia-Boente <jfernandez@igalia.com>
 *  Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
 *  Murray Cumming <murrayc@murrayc.com>
 *  Nils Faerber <nils@kernelconcepts.de>
 *  Florian Boor <florian@kernelconcepts.de>
 *  Silvan Marco Fin <silvan@kernelconcepts.de>
 *  Philip Van Hoof <philip@codeminded.be>
 *  Armin Burgmeier <armin@openismus.com>
 *  Arne Zellentin <arne@kernelconcepts.de>
 *  Johannes Schmid <johannes.schmid@openismus.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2006, Nokia Corporation
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

#include <im-service-mgr.h>

#include <im-account-mgr-helpers.h>
#include <im-error.h>

#include <string.h>
#include <glib/gi18n.h>
#include <gnome-keyring.h>
#include <gtk/gtk.h>

#define IM_OUTBOX_STORE_NAME "outboxes"
#define IM_LOCAL_STORE_NAME "local"

#define IM_LOCAL_DRAFTS_NAME "Drafts"

/* 'private'/'protected' functions */
static void    im_service_mgr_class_init   (ImServiceMgrClass *klass);
static void    im_service_mgr_finalize     (GObject *obj);
static void    im_service_mgr_instance_init (ImServiceMgr *obj);
static void    im_service_mgr_base_init     (gpointer g_class);

static void    on_account_inserted         (ImAccountMgr *acc_mgr,
					    const gchar *account,
					    gpointer user_data);

static void     add_existing_accounts       (ImServiceMgr *self);
static gboolean init_outbox                 (ImServiceMgr *self,
					     GError **error);
static gboolean init_store_outbox           (ImServiceMgr *self,
					     const char *name,
					     GError **error);
static gboolean remove_store_outbox         (ImServiceMgr *self,
					     const char *name,
					     GError **error);
static gboolean init_drafts                 (ImServiceMgr *self,
					     GError **error);
static gboolean init_local_inbox            (ImServiceMgr *self,
					     const gchar *account_name,
					     GError **error);
static gboolean remove_local_inbox          (ImServiceMgr *self,
					     const gchar *account_name,
					     GError **error);
static gboolean init_local_store            (ImServiceMgr *self,
					     GError **error);

static void    insert_account              (ImServiceMgr *self,
					    const gchar *account,
					    gboolean is_new);

static void    on_account_removed          (ImAccountMgr *acc_mgr, 
					    const gchar *account,
					    gpointer user_data);
static gboolean authenticate_sync          (CamelSession *session,
					    CamelService *service,
					    const gchar *mechanism,
					    GCancellable *cancellable,
					    GError **error);
static gchar * get_password                (CamelSession *session,
					    CamelService *service,
					    const gchar *prompt,
					    const gchar *item,
					    guint32 flags,
					    GError **error);
static gboolean forget_password	           (CamelSession *session,
					    CamelService *service,
					    const gchar *item,
					    GError **error);
static gchar * get_keyring_password        (CamelService *service,
					    guint32 flags);
static gboolean set_keyring_password       (CamelService *service,
					    const char *password);
static gint    alert_user                  (CamelSession *session,
					    CamelSessionAlertType type,
					    const gchar *prompt,
					    GSList *button_captions);
static CamelFilterDriver *get_filter_driver(CamelSession *session,
					    const gchar *type,
					    GError **error);
static gboolean forward_to                 (CamelSession *session,
					    CamelFolder *folder,
					    CamelMimeMessage *message,
					    const gchar *address,
					    GError **error);

/* list my signals */
enum {
	SERVICE_CHANGED_SIGNAL,
	SERVICE_INSERTED_SIGNAL,
	SERVICE_REMOVED_SIGNAL,

	LAST_SIGNAL
};

typedef struct _ImServiceMgrPrivate ImServiceMgrPrivate;
struct _ImServiceMgrPrivate {
	ImAccountMgr   *account_mgr;

	/* We cache the lists of accounts here */
	GHashTable          *store_services;
	GHashTable          *transport_services;

	/* Outboxes */
	CamelStore          *outbox_store;

	/* Local (drafts, sentbox, non storage inboxes) */
	CamelStore          *local_store;
};

#define IM_SERVICE_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									IM_TYPE_SERVICE_MGR, \
									ImServiceMgrPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GType
im_service_mgr_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImServiceMgrClass),
			im_service_mgr_base_init,	/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) im_service_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ImServiceMgr),
			0,		/* n_preallocs */
			(GInstanceInitFunc) im_service_mgr_instance_init,
			NULL
		};

		my_type = g_type_register_static (CAMEL_TYPE_SESSION,
 						  "ImServiceMgr",
						  &my_info, 0);
	}
	return my_type;
}


static void
im_service_mgr_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {

		signals[SERVICE_CHANGED_SIGNAL] =
			g_signal_new ("service_changed",
				      IM_TYPE_SERVICE_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ImServiceMgrClass, service_changed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, CAMEL_TYPE_SERVICE);

		signals[SERVICE_INSERTED_SIGNAL] =
			g_signal_new ("service_inserted",
				      IM_TYPE_SERVICE_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ImServiceMgrClass, service_inserted),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, CAMEL_TYPE_SERVICE);
		
		signals[SERVICE_REMOVED_SIGNAL] =
			g_signal_new ("service_removed",
				      IM_TYPE_SERVICE_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ImServiceMgrClass, service_removed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, CAMEL_TYPE_SERVICE);

		initialized = TRUE;
	}
}


static void
im_service_mgr_class_init (ImServiceMgrClass *klass)
{
	GObjectClass *gobject_class;
	CamelSessionClass *session_class;
	gobject_class = (GObjectClass*) klass;
	session_class = (CamelSessionClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = im_service_mgr_finalize;

	session_class->get_password = get_password;
	session_class->forget_password = forget_password;
	session_class->authenticate_sync = authenticate_sync;
	session_class->alert_user = alert_user;
	session_class->get_filter_driver = get_filter_driver;
	session_class->forward_to = forward_to;

	g_type_class_add_private (gobject_class,
				  sizeof(ImServiceMgrPrivate));
}

static void
im_service_mgr_instance_init (ImServiceMgr *obj)
{
	ImServiceMgrPrivate *priv;

	priv = IM_SERVICE_MGR_GET_PRIVATE(obj);

	priv->store_services = g_hash_table_new_full (g_str_hash, g_str_equal,
						      g_free, g_object_unref);
	priv->transport_services = g_hash_table_new_full (g_str_hash, g_str_equal,
							  g_free, g_object_unref);

	priv->account_mgr            = NULL;

}

static void
on_account_changed (ImAccountMgr *acc_mgr, 
		    const gchar *account_id, 
		    ImAccountType account_type,
		    gpointer user_data)
{
	ImServiceMgr *self = IM_SERVICE_MGR(user_data);
	ImServiceMgrPrivate *priv;
	GHashTable* account_hash;
	CamelService *service;

	priv = IM_SERVICE_MGR_GET_PRIVATE(self);
	account_hash = (account_type == IM_ACCOUNT_TYPE_STORE ? 
			priv->store_services : 
			priv->transport_services);

	service = (CamelService *) g_hash_table_lookup (account_hash, (char *) account_id);

	if (service) {
		/* TODO */
		/*modest_tny_account_update_from_account (tny_account, get_password, forget_password);*/
		g_signal_emit (G_OBJECT(self), signals[SERVICE_CHANGED_SIGNAL], 0, service);
	}
}

static gchar *
get_keyring_password (CamelService *service, guint32 flags)
{
	CamelSettings *settings;
	gchar *password = NULL;

	settings = camel_service_get_settings (service);
	if (CAMEL_IS_NETWORK_SETTINGS (settings)) {
		CamelNetworkSettings *network = (CamelNetworkSettings *) settings;
		CamelProvider *provider;
		GList *results = NULL;

		provider = camel_service_get_provider (service);
		if (GNOME_KEYRING_RESULT_OK == 
		    gnome_keyring_find_network_password_sync
		    (camel_network_settings_get_user (network),
		     NULL,
		     camel_network_settings_get_host (network),
		     NULL,
		     provider->protocol,
		     NULL,
		     camel_network_settings_get_port (network),
		     &results)) {
			if (results != NULL) {
				GnomeKeyringNetworkPasswordData *data = 
					(GnomeKeyringNetworkPasswordData *) results->data;

				password = g_strdup (data->password);
				gnome_keyring_network_password_list_free (results);
			}
		}

		if (password == NULL || flags & CAMEL_SESSION_PASSWORD_REPROMPT) {
			GtkWidget *dialog;
			GtkWidget *password_entry;

			dialog = gtk_message_dialog_new_with_markup
				(NULL, GTK_DIALOG_MODAL,
				 GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
				 _("<b>Need password</b>\nPassword needed for %s server at %s"),
				 provider->protocol,
				 camel_network_settings_get_host (network));

			password_entry = gtk_entry_new ();
			if (password) gtk_entry_set_text (GTK_ENTRY (password_entry), password);
			gtk_entry_set_visibility (GTK_ENTRY (password_entry), FALSE);
			gtk_container_add (GTK_CONTAINER (gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (dialog))),
					   password_entry);
			gtk_widget_show (password_entry);
			g_free (password);
			password = NULL;

			if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
				password = g_strdup (gtk_entry_get_text (GTK_ENTRY (password_entry)));
				set_keyring_password (service, password);
			}
			gtk_widget_destroy (dialog);
		}
	} else {
		g_return_val_if_reached (NULL);
	}

	return password;
}

static gboolean
set_keyring_password (CamelService *service, const char *password)
{
	CamelSettings *settings;

	settings = camel_service_get_settings (service);
	if (CAMEL_IS_NETWORK_SETTINGS (settings)) {
		CamelNetworkSettings *network = (CamelNetworkSettings *) settings;
		CamelProvider *provider;
		GList *results = NULL;

		provider = camel_service_get_provider (service);
		if (password != NULL) {
			guint32 item_id;
			gnome_keyring_set_network_password_sync
				(NULL,
				 camel_network_settings_get_user (network),
				 NULL,
				 camel_network_settings_get_host (network),
				 NULL,
				 provider->protocol,
				 NULL,
				 camel_network_settings_get_port (network),
				 password,
				 &item_id);
		} else {
			if (GNOME_KEYRING_RESULT_OK == 
			    gnome_keyring_find_network_password_sync
			    (camel_network_settings_get_user (network),
			     NULL,
			     camel_network_settings_get_host (network),
			     NULL,
			     provider->protocol,
			     NULL,
			     camel_network_settings_get_port (network),
			     &results)) {
				if (results != NULL) {
					GList *node;
					for (node = results; node != NULL; node = g_list_next (node)) {
						GnomeKeyringNetworkPasswordData *data = 
							(GnomeKeyringNetworkPasswordData *) node->data;
						gnome_keyring_item_delete_sync (data->keyring, data->item_id);
					}
					gnome_keyring_network_password_list_free (results);
				}
			}
		}
	} else {
		g_return_val_if_reached (FALSE);
	}
	return TRUE;
}

typedef struct _IdleGetPasswordData {
	CamelService *service;
	guint32 flags;
	gchar *password;
	GMutex mutex;
	GCond cond;
} IdleGetPasswordData;

static gboolean
idle_get_password (gpointer userdata)
{
	IdleGetPasswordData *data = (IdleGetPasswordData *) userdata;
	g_mutex_lock (&data->mutex);
	data->password = get_keyring_password (data->service, data->flags);
	g_cond_signal (&data->cond);
	g_mutex_unlock (&data->mutex);
	return FALSE;
}

static 	gchar *
get_password (CamelSession *session,
	      CamelService *service,
	      const gchar *prompt,
	      const gchar *item,
	      guint32 flags,
	      GError **error)
{
	gchar *password;

	if (g_main_context_is_owner (g_main_context_default ())) {
		password = get_keyring_password (service, flags);
	} else {
		IdleGetPasswordData *data = g_new0 (IdleGetPasswordData, 1);

		data->service = service;
		data->flags = flags;

		g_mutex_lock (&data->mutex);
		gdk_threads_add_idle (idle_get_password, data);
		g_cond_wait (&data->cond, &data->mutex);
	
		password = data->password;
		g_free (data);
	}

	return password;
}

static gboolean
forget_password	(CamelSession *session,
		 CamelService *service,
		 const gchar *item,
		 GError **error)
{
	gboolean result;

	gdk_threads_enter ();
	result = set_keyring_password (service, NULL);
	gdk_threads_leave ();

	return result;
}

static gboolean
authenticate_sync (CamelSession *session,
		   CamelService *service,
		   const gchar *mechanism,
		   GCancellable *cancellable,
		   GError **error)
{
	GError *_error = NULL;
	CamelAuthenticationResult auth_result;
	CamelServiceAuthType *auth_type = NULL;
	guint32 password_flags;

	if (mechanism != NULL) {
		auth_type = camel_sasl_authtype (mechanism);
		if (auth_type == NULL) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_AUTH_FAILED,
				     _("Unknown authentication mechanism %s"),
				     mechanism);
			goto finish;
		}
	}

	if (auth_type != NULL && !auth_type->need_password) {
		auth_result = camel_service_authenticate_sync (service, 
							       mechanism, 
							       cancellable, error);
		if (auth_result != CAMEL_AUTHENTICATION_ACCEPTED) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_AUTH_FAILED,
				     _("Failed to authenticate using %s without password"),
				     mechanism);
		}
		goto finish;
	}

	if (mechanism != NULL) {
		CamelProvider *provider;
		CamelSasl *sasl;
		GError *sasl_error= NULL;

		provider = camel_service_get_provider (service);
		sasl = camel_sasl_new (provider->protocol, mechanism, service);
		if (sasl != NULL) {
			gboolean success;
			success = camel_sasl_try_empty_password_sync (sasl, cancellable, &sasl_error);
			g_object_unref (sasl);
			if (g_error_matches (sasl_error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
				g_propagate_error (&_error, sasl_error);
				goto finish;
			} else if (success) {
				auth_result = CAMEL_AUTHENTICATION_ACCEPTED;
				goto finish;
			} else {
				if (sasl_error) g_error_free (sasl_error);
			}
		}
	}

	password_flags = CAMEL_SESSION_PASSWORD_SECRET;
	while (TRUE) {
		const char *password;

		password = camel_service_get_password (service);
		if (password == NULL) {
			gchar *new_password;

			new_password = camel_session_get_password (session, service, "", "password",
								   password_flags, &_error);
			camel_service_set_password (service, new_password);
			g_free (new_password);

			if (new_password == NULL && _error == NULL) {
				g_set_error (&_error, IM_ERROR_DOMAIN,
					     IM_ERROR_AUTH_FAILED,
					     _("No password provided"));
			}
			if (_error) goto finish;
		}

		auth_result = camel_service_authenticate_sync (service, mechanism, 
							       cancellable, &_error);

		if (auth_result == CAMEL_AUTHENTICATION_REJECTED) {
			password_flags |= CAMEL_SESSION_PASSWORD_REPROMPT;
			camel_service_set_password (service, NULL);
		} else {
			break;
		}
	}

 finish:
	if (_error) {
		g_propagate_error (error, _error);
		return FALSE;
	} else {
		return (auth_result == CAMEL_AUTHENTICATION_ACCEPTED);
	}
}

static gint
alert_user (CamelSession *session,
	    CamelSessionAlertType type,
	    const gchar *prompt,
	    GSList *button_captions)
{
	return -1;
}

static CamelFilterDriver *
get_filter_driver (CamelSession *session,
		   const gchar *type,
		   GError **error)
{
	return NULL;
}

static gboolean
forward_to (CamelSession *session,
	    CamelFolder *folder,
	    CamelMimeMessage *message,
	    const gchar *address,
	    GError **error)
{
	return FALSE;
}


static void
im_service_mgr_finalize (GObject *obj)
{
	ImServiceMgr *self        = IM_SERVICE_MGR(obj);
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE(self);

	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	/* Destroy all accounts. Disconnect all accounts before they are destroyed */
	if (priv->store_services) {
		g_hash_table_destroy (priv->store_services);
		priv->store_services = NULL;
	}
	
	if (priv->transport_services) {
		g_hash_table_destroy (priv->transport_services);
		priv->transport_services = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
disconnect_service (gpointer key,
		    gpointer value,
		    gpointer userdata)
{
	CamelService *service = (CamelService *) value;

	if (camel_service_get_connection_status (service) != CAMEL_SERVICE_DISCONNECTED) {
		camel_service_disconnect_sync (service, FALSE, NULL);
	}
}

static void
disconnect_all (ImServiceMgr *self)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE(self);

	g_hash_table_foreach (priv->store_services,
			      disconnect_service,
			      NULL);
	g_hash_table_foreach (priv->transport_services,
			      disconnect_service,
			      NULL);
}

static void
on_network_changed (GNetworkMonitor *monitor,
		    gboolean available,
		    gpointer userdata)
{
	ImServiceMgr *self = (ImServiceMgr *) userdata;

	camel_session_set_online (CAMEL_SESSION (self),
				  available);

	if (!available) {
		disconnect_all (self);
	}
}

static ImServiceMgr*
im_service_mgr_new (ImAccountMgr *account_mgr)
{
	GObject *obj;
	ImServiceMgrPrivate *priv;
	char *user_cache_dir;
	GNetworkMonitor *monitor;

	g_return_val_if_fail (account_mgr, NULL);

	user_cache_dir = g_build_filename (g_get_user_cache_dir (), "iwkmail", NULL);

	obj  = G_OBJECT(g_object_new(IM_TYPE_SERVICE_MGR,
				     "user-data-dir", im_service_mgr_get_user_data_dir (),
				     "user-cache-dir", user_cache_dir,
				     NULL));
	g_free (user_cache_dir);

	camel_session_set_junk_headers (CAMEL_SESSION (obj), NULL, NULL, 0);

	priv = IM_SERVICE_MGR_GET_PRIVATE(obj);

	monitor = g_network_monitor_get_default ();
	g_signal_connect (G_OBJECT (monitor), "network-changed",
			  G_CALLBACK (on_network_changed), obj);
	on_network_changed (monitor,
			    g_network_monitor_get_network_available (monitor),
			    (gpointer) obj);

	priv->account_mgr = g_object_ref (G_OBJECT(account_mgr));

	/* Connect signals */
	g_signal_connect (G_OBJECT(account_mgr), "account_inserted",
			  G_CALLBACK (on_account_inserted), obj);
	g_signal_connect (G_OBJECT(account_mgr), "account_changed",
			  G_CALLBACK (on_account_changed), obj);
	g_signal_connect (G_OBJECT(account_mgr), "account_removed",
			  G_CALLBACK (on_account_removed), obj);

	/* Add the other remote accounts. Do this after adding the
	   local account, because we need to add our outboxes to the
	   global OUTBOX hosted in the local account */
	add_existing_accounts (IM_SERVICE_MGR (obj));

	return IM_SERVICE_MGR(obj);
}

ImServiceMgr *
im_service_mgr_get_instance (void)
{
	static ImServiceMgr *instance = 0;

	if (instance == 0)
		instance = im_service_mgr_new (im_account_mgr_get_instance ());

	return instance;
}

CamelService *
im_service_mgr_get_service (ImServiceMgr *self,
			    const gchar *account_id,
			    ImAccountType type)
{
	ImServiceMgrPrivate *priv = NULL;
	CamelService *retval = NULL;
	GHashTable *account_hash;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_id, NULL);
	g_return_val_if_fail (type == IM_ACCOUNT_TYPE_STORE || 
			      type == IM_ACCOUNT_TYPE_TRANSPORT,
			      NULL);
	
	priv = IM_SERVICE_MGR_GET_PRIVATE(self);

	account_hash = (type == IM_ACCOUNT_TYPE_STORE) ? 
		priv->store_services : 
		priv->transport_services;

	if (!account_hash) {
		g_printerr ("%s: No server accounts of type %s\n", __FUNCTION__, 
			(type == IM_ACCOUNT_TYPE_STORE) ? "store" : "transport");
		return NULL;
	}

	retval = g_hash_table_lookup (account_hash, account_id);

	if (retval == NULL) {
		g_printerr ("%s: could not get %s service for %s\n.", __FUNCTION__,
			    (type == IM_ACCOUNT_TYPE_STORE) ? "store" : "transport",
			    account_id);
	}
	
	/* Returns a new reference */
	return retval;
}

/*********************************************************************************/
static void
add_existing_accounts (ImServiceMgr *self)
{
	GSList *account_ids = NULL, *iter = NULL;
	ImServiceMgrPrivate *priv = NULL;
	
	priv = IM_SERVICE_MGR_GET_PRIVATE(self);

	/* These are account names, not server_account names */
	account_ids = im_account_mgr_get_account_ids (priv->account_mgr, FALSE);

	for (iter = account_ids; iter != NULL; iter = g_slist_next (iter)) {
		const gchar *account_id = (const gchar*) iter->data;
		
		/* Insert all enabled accounts without notifying */
		if (im_account_mgr_get_enabled (priv->account_mgr, account_id))
			insert_account (self, account_id, FALSE);
	}
	im_account_mgr_free_account_ids (account_ids);
}

static void
fill_network_settings (ImServerAccountSettings *server,
		       ImAccountType account_type,
		       CamelNetworkSettings *network)
{
	ImProtocolType connection;
	CamelNetworkSecurityMethod security_method;

	if (account_type == IM_ACCOUNT_TYPE_TRANSPORT) {
		ImProtocolType auth;
		const char *auth_mech = NULL;
		auth = im_server_account_settings_get_auth_protocol (server);
		if (auth == im_protocol_registry_get_password_auth_type_id ())
			auth_mech = "LOGIN";
		else if (auth == im_protocol_registry_get_crammd5_auth_type_id ()) {
			auth_mech = "CRAM-MD5";
		}
		
		if (auth_mech) {
			camel_network_settings_set_auth_mechanism (network, auth_mech);
		}
	}

	connection = im_server_account_settings_get_security_protocol (server);
	if (connection == im_protocol_registry_get_ssl_connection_type_id ())
		security_method = CAMEL_NETWORK_SECURITY_METHOD_SSL_ON_ALTERNATE_PORT;
	else if (connection == im_protocol_registry_get_tls_connection_type_id ())
		security_method = CAMEL_NETWORK_SECURITY_METHOD_STARTTLS_ON_STANDARD_PORT;
	else
		security_method = CAMEL_NETWORK_SECURITY_METHOD_NONE;
	camel_network_settings_set_security_method (network,
						    security_method);

	camel_network_settings_set_host (network,
					 im_server_account_settings_get_hostname (server));
	camel_network_settings_set_port (network,
					 im_server_account_settings_get_port (server));
	camel_network_settings_set_user (network,
					 im_server_account_settings_get_username (server));
}

static gboolean
init_outbox (ImServiceMgr *self, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	GError *_error = NULL;

	if (priv->outbox_store != NULL)
		return TRUE;

	priv->outbox_store = (CamelStore *)camel_session_add_service (CAMEL_SESSION (self),
								      IM_OUTBOX_STORE_NAME,
								      "maildir",
								      CAMEL_PROVIDER_STORE,
								      &_error);

	if (priv->outbox_store) {
		CamelSettings *settings;

		g_object_set (priv->outbox_store,
			      "need-summary-check", TRUE,
			      NULL);

		settings = camel_service_get_settings (CAMEL_SERVICE (priv->outbox_store));
		if (CAMEL_IS_LOCAL_SETTINGS (settings)) {
			gchar *path = g_build_filename (im_service_mgr_get_user_data_dir (),
							IM_OUTBOX_STORE_NAME, NULL);
			camel_local_settings_set_path (CAMEL_LOCAL_SETTINGS (settings),
						       path);
			g_free (path);
		}
	}

	if (_error)
		g_propagate_error (error, _error);

	return priv->outbox_store != NULL;
}

static gboolean
init_store_outbox (ImServiceMgr *self, const gchar *name, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	GError *_error = NULL;

	if (!init_outbox (self, &_error)) {
		if (_error) g_propagate_error (error, _error);
		return FALSE;
	}

	fi = camel_store_get_folder_info_sync (priv->outbox_store,
					       name,
					       0,
					       NULL, NULL);
	if (fi) {
		camel_store_free_folder_info (priv->outbox_store, fi);
		return TRUE;
	}

	fi = camel_store_create_folder_sync (priv->outbox_store,
					     NULL, name,
					     NULL, &_error);

	if (fi) {
		camel_store_free_folder_info (priv->outbox_store, fi);
		return TRUE;
	}

	if (_error)
		g_propagate_error (error, _error);

	return FALSE;
}

static gboolean
remove_store_outbox (ImServiceMgr *self, const gchar *name, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	GError *_error = NULL;

	if (!init_outbox (self, &_error)) {
		if (_error) g_propagate_error (error, _error);
		return FALSE;
	}

	fi = camel_store_get_folder_info_sync (priv->outbox_store,
					       name,
					       0,
					       NULL, NULL);
	if (fi) {
		camel_store_free_folder_info (priv->outbox_store, fi);
		if (camel_store_delete_folder_sync (priv->outbox_store, name,
						    NULL, &_error))
			return TRUE;
	}

	if (_error)
		g_propagate_error (error, _error);

	return FALSE;
}

CamelFolder *
im_service_mgr_get_outbox (ImServiceMgr *self,
			   const char *account_id,
			   GCancellable *cancellable,
			   GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	if (!init_store_outbox (self, account_id, error))
		return NULL;

	return camel_store_get_folder_sync (priv->outbox_store,
					    account_id,
					    0,
					    cancellable, error);
}

CamelStore *
im_service_mgr_get_outbox_store (ImServiceMgr *self)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	if (!init_outbox (self, NULL))
		return NULL;

	return priv->outbox_store;
}
						
static gboolean
init_local_store (ImServiceMgr *self, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	GError *_error = NULL;

	if (priv->local_store != NULL)
		return TRUE;

	priv->local_store = (CamelStore *)camel_session_add_service (CAMEL_SESSION (self),
								      IM_LOCAL_STORE_NAME,
								      "maildir",
								      CAMEL_PROVIDER_STORE,
								      &_error);

	if (priv->local_store) {
		CamelSettings *settings;

		g_object_set (priv->local_store,
			      "need-summary-check", TRUE,
			      NULL);

		settings = camel_service_get_settings (CAMEL_SERVICE (priv->local_store));
		if (CAMEL_IS_LOCAL_SETTINGS (settings)) {
			gchar *path = g_build_filename (im_service_mgr_get_user_data_dir (),
							IM_LOCAL_STORE_NAME, NULL);
			camel_local_settings_set_path (CAMEL_LOCAL_SETTINGS (settings),
						       path);
			g_free (path);
		}
	}

	if (_error)
		g_propagate_error (error, _error);

	return priv->local_store != NULL;
}

static gboolean
init_drafts (ImServiceMgr *self, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	GError *_error = NULL;

	if (!init_local_store (self, &_error)) {
		if (_error) g_propagate_error (error, _error);
		return FALSE;
	}

	fi = camel_store_get_folder_info_sync (priv->local_store,
					       IM_LOCAL_DRAFTS_NAME,
					       0,
					       NULL, NULL);
	if (fi) {
		camel_store_free_folder_info (priv->local_store, fi);
		return TRUE;
	}

	fi = camel_store_create_folder_sync (priv->local_store,
					     NULL, IM_LOCAL_DRAFTS_NAME,
					     NULL, &_error);

	if (fi) {
		camel_store_free_folder_info (priv->local_store, fi);
		return TRUE;
	}

	if (_error)
		g_propagate_error (error, _error);

	return FALSE;
}

static gboolean
init_local_inbox (ImServiceMgr *self, const gchar *account_name, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	GError *_error = NULL;

	if (!init_local_store (self, &_error)) {
		if (_error) g_propagate_error (error, _error);
		return FALSE;
	}

	fi = camel_store_get_folder_info_sync (priv->local_store,
					       account_name,
					       0,
					       NULL, NULL);
	if (fi) {
		camel_store_free_folder_info (priv->local_store, fi);
		return TRUE;
	}

	fi = camel_store_create_folder_sync (priv->local_store,
					     NULL, account_name,
					     NULL, &_error);

	if (fi) {
		camel_store_free_folder_info (priv->local_store, fi);
		return TRUE;
	}

	if (_error)
		g_propagate_error (error, _error);

	return FALSE;
}

static gboolean
remove_local_inbox (ImServiceMgr *self, const gchar *account_name, GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	GError *_error = NULL;

	if (!init_local_store (self, &_error)) {
		if (_error) g_propagate_error (error, _error);
		return FALSE;
	}

	fi = camel_store_get_folder_info_sync (priv->local_store,
					       account_name,
					       0,
					       NULL, NULL);
	if (fi) {
		camel_store_free_folder_info (priv->local_store, fi);
		if (camel_store_delete_folder_sync (priv->outbox_store, account_name,
						    NULL, &_error))
			return TRUE;
	}

	if (_error)
		g_propagate_error (error, _error);

	return FALSE;
}

CamelFolder *
im_service_mgr_get_drafts (ImServiceMgr *self,
			   GCancellable *cancellable,
			   GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	if (!init_drafts (self, error))
		return NULL;

	return camel_store_get_folder_sync (priv->local_store,
					    IM_LOCAL_DRAFTS_NAME,
					    0,
					    cancellable, error);
}

gboolean
im_service_mgr_has_local_inbox (ImServiceMgr *self,
				const char *account_name)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelFolderInfo *fi;
	gboolean result;

	if (!init_local_store (self, NULL))
		return FALSE;

	fi = camel_store_get_folder_info_sync (priv->local_store,
					       account_name, 0, NULL, NULL);

	result = (fi != NULL);
	camel_store_free_folder_info (priv->local_store, fi);

	return result;
}

CamelFolder *
im_service_mgr_get_local_inbox (ImServiceMgr *self,
				const gchar *account_name,
				GCancellable *cancellable,
				GError **error)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	if (!init_local_inbox (self, account_name, error))
		return NULL;

	return camel_store_get_folder_sync (priv->local_store,
					    account_name,
					    0,
					    cancellable, error);
}

CamelStore *
im_service_mgr_get_local_store (ImServiceMgr *self)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	if (!init_local_store (self, NULL))
		return NULL;

	return priv->local_store;
}

CamelFolder *
im_service_mgr_get_folder (ImServiceMgr *self,
			   const gchar *account_id,
			   const gchar *folder_name,
			   GCancellable *cancellable,
			   GError **error)
{
	CamelFolder *folder;
	if (g_strcmp0 (folder_name, "INBOX") == 0 && 
	    im_service_mgr_has_local_inbox (self,
					    account_id)) {
		folder = im_service_mgr_get_local_inbox (self,
							 account_id,
							 cancellable,
							 error);
	} else if (g_strcmp0 (folder_name, IM_LOCAL_DRAFTS_TAG) == 0) {
		folder = im_service_mgr_get_drafts (self,
						    cancellable,
						    error);
	} else if (g_strcmp0 (folder_name, IM_LOCAL_OUTBOX_TAG) == 0) {
		folder = im_service_mgr_get_outbox (self,
						    account_id,
						    cancellable,
						    error);
	} else {
		CamelStore *store;
		store = (CamelStore *) im_service_mgr_get_service (self,
								   account_id,
								   IM_ACCOUNT_TYPE_STORE);
		folder = camel_store_get_folder_sync 
			(store, folder_name,
			 CAMEL_STORE_FOLDER_CREATE | CAMEL_STORE_FOLDER_BODY_INDEX, 
			 cancellable,
			 error);
	}

	return folder;
}
						

static CamelService*
create_service (ImServiceMgr *self,
		const gchar *name,
		ImAccountType type,
		gboolean notify)
{
	ImServiceMgrPrivate *priv = IM_SERVICE_MGR_GET_PRIVATE (self);
	CamelService *service;
	ImAccountSettings *account_settings;
	ImServerAccountSettings *server_settings;
	ImProtocolType protocol_type;
	ImProtocol *protocol;
	const char *protocol_str;

	account_settings = im_account_mgr_load_account_settings (priv->account_mgr, name);
	server_settings = (type == IM_ACCOUNT_TYPE_STORE)?
		im_account_settings_get_store_settings (account_settings):
		im_account_settings_get_transport_settings (account_settings);

	protocol_type = im_server_account_settings_get_protocol (server_settings);
	protocol = im_protocol_registry_get_protocol_by_type (im_protocol_registry_get_instance (),
							      protocol_type);
	g_return_val_if_fail (protocol, NULL);
	protocol_str = im_protocol_get_name (protocol);
	service = camel_session_add_service (CAMEL_SESSION (self),
					     im_server_account_settings_get_account_name (server_settings),
					     protocol_str,
					     (type == IM_ACCOUNT_TYPE_STORE)?CAMEL_PROVIDER_STORE:CAMEL_PROVIDER_TRANSPORT, NULL);

	if (service) {
		CamelSettings *settings;
		
		settings = camel_service_get_settings (service);
		if (CAMEL_IS_NETWORK_SETTINGS (settings))
			fill_network_settings (server_settings, type, CAMEL_NETWORK_SETTINGS (settings));

		if (type == IM_ACCOUNT_TYPE_STORE) {
			CamelProvider *provider;
			provider = camel_service_get_provider (service);
			if (!(provider->flags & CAMEL_PROVIDER_IS_STORAGE))
				init_local_inbox (self, name, NULL);
		}
		
	}

	init_store_outbox (self, name, NULL);

	g_object_unref (server_settings);
	g_object_unref (account_settings);
	return service;
}


/*
 * This function will be used for both adding new accounts and for the
 * initialization. In the initialization we do not want to emit
 * signals so notify will be FALSE, in the case of account additions
 * we do want to notify the observers
 */
static void
insert_account (ImServiceMgr *self,
		const gchar *account,
		gboolean is_new)
{
	ImServiceMgrPrivate *priv = NULL;
	CamelService *store_service = NULL, *transport_service = NULL;

	priv = IM_SERVICE_MGR_GET_PRIVATE(self);

	/* Get the server and the transport account */
	store_service = create_service (self, account, IM_ACCOUNT_TYPE_STORE, is_new);
	if (!store_service || !CAMEL_IS_STORE(store_service)) {
		g_warning ("%s: failed to create store account", __FUNCTION__);
		return;
	}

	transport_service = create_service (self, account, IM_ACCOUNT_TYPE_TRANSPORT, is_new);
	if (!transport_service || !CAMEL_IS_TRANSPORT(transport_service)) {
		g_warning ("%s: failed to create transport account", __FUNCTION__);
		g_object_unref (store_service);
		return;
	}

	g_hash_table_insert (priv->store_services, g_strdup (account), store_service);
	g_hash_table_insert (priv->transport_services, g_strdup (account), transport_service);
}

static void
on_account_inserted (ImAccountMgr *acc_mgr, 
		     const gchar *account,
		     gpointer user_data)
{
	/* Insert the account and notify the observers */
	insert_account (IM_SERVICE_MGR (user_data), account, TRUE);

}

static void
on_account_removed (ImAccountMgr *acc_mgr, 
		    const gchar *account,
		    gpointer user_data)
{
	CamelService *store_service = NULL, *transport_service = NULL;
	ImServiceMgr *self;
	ImServiceMgrPrivate *priv;
	
	self = IM_SERVICE_MGR (user_data);
	priv = IM_SERVICE_MGR_GET_PRIVATE (self);

	/* Get the server and the transport account */
	store_service = 
		im_service_mgr_get_service (self, account, 
					    IM_ACCOUNT_TYPE_STORE);
	transport_service = 
		im_service_mgr_get_service (self, account,
					    IM_ACCOUNT_TYPE_TRANSPORT);
	
	if (CAMEL_IS_STORE (store_service)) {
		camel_service_disconnect_sync (store_service, TRUE, NULL);
		g_hash_table_remove (priv->store_services, store_service);
	} else {
		g_warning ("%s: no store account for account %s\n", 
			   __FUNCTION__, account);
	}

	if (CAMEL_IS_TRANSPORT (transport_service)) {
		camel_service_disconnect_sync (transport_service, TRUE, NULL);
		g_hash_table_remove (priv->transport_services, transport_service);
	} else {
		g_warning ("%s: no transport account for account %s\n", 
			   __FUNCTION__, account);
	}

	remove_local_inbox (self, account, NULL);
	remove_store_outbox (self, account, NULL);

}

const char *
im_service_mgr_get_user_data_dir (void)
{
	static char *user_data_dir;

	if (user_data_dir == 0)
		user_data_dir = g_build_filename (g_get_user_data_dir (), "iwkmail", NULL);

	return user_data_dir;
}
