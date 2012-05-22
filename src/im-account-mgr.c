/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-mgr.c : account settings persistent storage */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Felipe Erias Morandeira <femorandeira@igalia.com>
 *  Dirk-Jan C. Binnema
 *  Murray Cumming
 *  Silvan Marco Fin
 *  Arne Zellentin
 *  Florian Boor
 *  Johannes Schmid
 *  Nils Faerber
 *  Philip Van Hoof
 *  Alexander Chumakov
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

#include <im-account-mgr.h>
#include <im-account-mgr-priv.h>

#include <im-account-mgr-helpers.h>

#include <string.h>

/* 'private'/'protected' functions */
static void im_account_mgr_class_init (ImAccountMgrClass * klass);
static void im_account_mgr_init       (ImAccountMgr * obj);
static void im_account_mgr_finalize   (GObject * obj);
static void im_account_mgr_base_init  (gpointer g_class);

static const gchar *_im_account_mgr_get_account_keyname_cached (ImAccountMgrPrivate *priv, 
								    const gchar* account_name,
								    const gchar *name, 
								    gboolean is_server);

static gboolean im_account_mgr_unset_default_account (ImAccountMgr *self);

/* list my signals */
enum {
	ACCOUNT_INSERTED_SIGNAL,
	ACCOUNT_CHANGED_SIGNAL,
	ACCOUNT_REMOVED_SIGNAL,
	ACCOUNT_BUSY_SIGNAL,
	DEFAULT_ACCOUNT_CHANGED_SIGNAL,
	DISPLAY_NAME_CHANGED_SIGNAL,
	ACCOUNT_UPDATED_SIGNAL,
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = {0};

GType
im_account_mgr_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof (ImAccountMgrClass),
			im_account_mgr_base_init,	/* base init */
			NULL,	/* base finalize */
			(GClassInitFunc) im_account_mgr_class_init,
			NULL,	/* class finalize */
			NULL,	/* class data */
			sizeof (ImAccountMgr),
			1,	/* n_preallocs */
			(GInstanceInitFunc) im_account_mgr_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImAccountMgr",
						  &my_info, 0);
	}
	return my_type;
}

static void 
im_account_mgr_base_init (gpointer g_class)
{
	static gboolean im_account_mgr_initialized = FALSE;

	if (!im_account_mgr_initialized) {
		/* signal definitions */
		signals[ACCOUNT_INSERTED_SIGNAL] =
			g_signal_new ("account_inserted",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, account_inserted),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);

		signals[ACCOUNT_REMOVED_SIGNAL] =
			g_signal_new ("account_removed",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, account_removed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);

		signals[ACCOUNT_CHANGED_SIGNAL] =
			g_signal_new ("account_changed",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, account_changed),
				      NULL, NULL,
				      g_cclosure_marshal_generic,
				      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT);

		signals[ACCOUNT_BUSY_SIGNAL] =
			g_signal_new ("account_busy_changed",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, account_busy_changed),
				      NULL, NULL,
				      g_cclosure_marshal_generic,
				      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);

		signals[DEFAULT_ACCOUNT_CHANGED_SIGNAL] =
			g_signal_new ("default_account_changed",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, default_account_changed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);

		signals[DISPLAY_NAME_CHANGED_SIGNAL] =
			g_signal_new ("display_name_changed",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, display_name_changed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);
		
		signals[ACCOUNT_UPDATED_SIGNAL] =
			g_signal_new ("account_updated",
				      IM_TYPE_ACCOUNT_MGR,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ImAccountMgrClass, account_updated),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);


		im_account_mgr_initialized = TRUE;
	}
}

static void
im_account_mgr_class_init (ImAccountMgrClass * klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = im_account_mgr_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof (ImAccountMgrPrivate));
}


static void
im_account_mgr_init (ImAccountMgr * obj)
{
	ImAccountMgrPrivate *priv =
		IM_ACCOUNT_MGR_GET_PRIVATE (obj);

	priv->im_conf   = NULL;
	priv->busy_accounts = NULL;
	priv->timeout       = 0;
	
	priv->notification_id_accounts = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_free);

	/* we maintain hashes for the im-conf keys we build from account name
	 * + key. many seem to be used often, and generating them showed up high
	 * in oprofile */
	/* both hashes are hashes to hashes;
	 * account-key => keyname ==> account-key-name
	 */	
	priv->server_account_key_hash = g_hash_table_new_full (g_str_hash,
							       g_str_equal,
							       g_free,
							       (GDestroyNotify)g_hash_table_destroy);
	priv->account_key_hash        = g_hash_table_new_full (g_str_hash,
							       g_str_equal,
							       g_free,
							       (GDestroyNotify)g_hash_table_destroy);

	/* FALSE means: status is unknown */
	priv->has_accounts = FALSE;
	priv->has_enabled_accounts = FALSE;
}

static void
im_account_mgr_finalize (GObject * obj)
{
	ImAccountMgrPrivate *priv = 
		IM_ACCOUNT_MGR_GET_PRIVATE (obj);

	if (priv->notification_id_accounts) {
		g_hash_table_destroy (priv->notification_id_accounts);
		priv->notification_id_accounts = NULL;
	}

	if (priv->im_conf) {
		g_object_unref (G_OBJECT(priv->im_conf));
		priv->im_conf = NULL;
	}

	if (priv->timeout)
		g_source_remove (priv->timeout);
	priv->timeout = 0;

	if (priv->server_account_key_hash) {
		g_hash_table_destroy (priv->server_account_key_hash);
		priv->server_account_key_hash = NULL;
	}
	
	if (priv->account_key_hash) {
		g_hash_table_destroy (priv->account_key_hash);
		priv->account_key_hash = NULL;
	}

	if (priv->busy_accounts) {
		g_slist_foreach (priv->busy_accounts, (GFunc) g_free, NULL);
		g_slist_free (priv->busy_accounts);
		priv->busy_accounts = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static ImAccountMgr *
im_account_mgr_new (ImConf *conf)
{
	GObject *obj;
	ImAccountMgrPrivate *priv;

	g_return_val_if_fail (conf, NULL);

	obj = G_OBJECT (g_object_new (IM_TYPE_ACCOUNT_MGR, NULL));
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (obj);

	g_object_ref (G_OBJECT(conf));
	priv->im_conf = conf;

	return IM_ACCOUNT_MGR (obj);
}

ImAccountMgr *
im_account_mgr_get_instance (void)
{
	static ImAccountMgr *mgr = 0;
	if (mgr == 0) {
		mgr = im_account_mgr_new (im_conf_get_instance ());
	}

	return mgr;
}


static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}

gboolean
im_account_mgr_add_account_from_settings (ImAccountMgr *self,
					  ImAccountSettings *settings)
{
	const gchar* display_name;
	gchar *account_name_start, *account_name;
	gchar *store_name_start, *store_name;
	gchar *transport_name_start, *transport_name;
	gchar *default_account;
	ImServerAccountSettings *store_settings, *transport_settings;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR (self), FALSE);
	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), FALSE);

	display_name = im_account_settings_get_display_name (settings);

	/* We should have checked for this already, and changed that name accordingly, 
	 * but let's check again just in case */
	if (!display_name || 
	    im_account_mgr_account_with_display_name_exists (self, display_name)) {
		display_name = _("Account");
	}

	/* Increment the non-user visible name if necessary, 
	 * based on the display name: */
	account_name_start = g_strdup_printf ("%sID", display_name);
	account_name = im_account_mgr_get_unused_account_name (self,
								   account_name_start, FALSE /* not a server account */);
	g_free (account_name_start);
	
	/* Add a (incoming) server account, to be used by the account: */
	store_name_start = g_strconcat (account_name, "_store", NULL);
	store_name = im_account_mgr_get_unused_account_name (self, 
								 store_name_start, TRUE /* server account */);
	g_free (store_name_start);
	
	/* Add a (outgoing) server account to be used by the account: */
	transport_name_start = g_strconcat (account_name, "_transport", NULL);
	transport_name = im_account_mgr_get_unused_account_name (self, 
								     transport_name_start, TRUE /* server account */);
	g_free (transport_name_start);

	im_account_settings_set_id (settings, account_name);
	store_settings = im_account_settings_get_store_settings (settings);
	im_server_account_settings_set_account_name (store_settings, store_name);
	transport_settings = im_account_settings_get_transport_settings (settings);
	im_server_account_settings_set_account_name (transport_settings, transport_name);
	g_object_unref (store_settings);
	g_object_unref (transport_settings);

	/* Create the account, which will contain the two "server accounts": */
 	im_account_mgr_save_account_settings (self, settings);
	g_free (store_name);
	g_free (transport_name);
	
	/* Sanity check: */
	/* There must be at least one account now: */
	/* Note, when this fails is is caused by a Maemo gconf bug that has been 
	 * fixed in versions after 3.1. */
	if(!im_account_mgr_has_accounts (self, FALSE))
		g_warning (_("im_account_mgr_get_account_ids() returned NULL after adding an account."));
				
	/* Notify the observers */
	g_signal_emit (self, signals[ACCOUNT_INSERTED_SIGNAL], 0, account_name);

	/* if no default account has been defined yet, do so now */
	default_account = im_account_mgr_get_default_account (self);
	if (!default_account) {
		im_account_mgr_set_default_account (self, account_name);
		im_account_settings_set_is_default (settings, TRUE);
	}
	g_free (default_account);
	g_free (account_name);

	/* (re)set the automatic account update */
#if 0
	/* TODO: reenable update interval */
	im_platform_set_update_interval
		(im_conf_get_int (priv->im_conf, IM_CONF_UPDATE_INTERVAL, NULL));
#endif

	return TRUE;
}


gboolean
im_account_mgr_add_account (ImAccountMgr *self,
				const gchar *name,
				const gchar *display_name,
				const gchar *user_fullname,
				const gchar *user_email,
				ImAccountRetrieveType retrieve_type,
				const gchar *store_account,
				const gchar *transport_account,
				gboolean enabled)
{
	ImAccountMgrPrivate *priv;
	const gchar *key;
	gboolean ok;
	gchar *default_account;
	GError *err = NULL;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (strchr(name, '/') == NULL, FALSE);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	/*
	 * we create the account by adding an account 'dir', with the name <name>,
	 * and in that the 'display_name' string key
	 */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_DISPLAY_NAME,
							      FALSE);
	if (im_account_mgr_account_exists (self, key, FALSE)) {
		g_printerr (_("im: account already exists\n"));
		return FALSE;
	}
	
	ok = im_conf_set_string (priv->im_conf, key, name, &err);
	if (!ok) {
		g_printerr (_("im: cannot set display name\n"));
		if (err) {
			g_printerr (_("im: Error adding account conf: %s\n"), err->message);
			g_error_free (err);
		}
		return FALSE;
	}
	
	if (store_account) {
		key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_STORE_ACCOUNT,
								      FALSE);
		ok = im_conf_set_string (priv->im_conf, key, store_account, &err);
		if (!ok) {
			g_printerr (_("im: failed to set store account '%s'\n"),
				    store_account);
			if (err) {
				g_printerr (_("im: Error adding store account conf: %s\n"), err->message);
				g_error_free (err);
			}
			return FALSE;
		}
	}
	
	if (transport_account) {
		key = _im_account_mgr_get_account_keyname_cached (priv, name,
								      IM_ACCOUNT_TRANSPORT_ACCOUNT,
								      FALSE);
		ok = im_conf_set_string (priv->im_conf, key, transport_account, &err);
		if (!ok) {
			g_printerr (_("im: failed to set transport account '%s'\n"),
				    transport_account);
			if (err) {
				g_printerr (_("im: Error adding transport account conf: %s\n"), err->message);
				g_error_free (err);
			}	
			return FALSE;
		}
	}

	/* Make sure that leave-messages-on-server is enabled by default, 
	 * as per the UI spec, though it is only meaningful for accounts using POP.
	 * (possibly this gconf key should be under the server account): */
	im_account_mgr_set_bool (self, name, IM_ACCOUNT_LEAVE_ON_SERVER, TRUE, FALSE);
	im_account_mgr_set_bool (self, name, IM_ACCOUNT_ENABLED, enabled,FALSE);

	/* Fill other data */
	im_account_mgr_set_string (self, name,
				       IM_ACCOUNT_DISPLAY_NAME, 
				       display_name, FALSE);
	im_account_mgr_set_string (self, name,
				       IM_ACCOUNT_FULLNAME, 
				       user_fullname, FALSE);
	im_account_mgr_set_string (self, name,
				       IM_ACCOUNT_EMAIL, 
				       user_email, FALSE);
	im_account_mgr_set_retrieve_type (self, name,
					      retrieve_type);

	/* Notify the observers */
	g_signal_emit (self, signals[ACCOUNT_INSERTED_SIGNAL], 0, name);

	/* if no default account has been defined yet, do so now */
	default_account = im_account_mgr_get_default_account (self);
	if (!default_account)
		im_account_mgr_set_default_account (self, name);
	g_free (default_account);
	
#if 0
	/* (re)set the automatic account update */
	im_platform_set_update_interval
		(im_conf_get_int (priv->im_conf, IM_CONF_UPDATE_INTERVAL, NULL));
#endif
	
	return TRUE;
}


gboolean
im_account_mgr_add_server_account (ImAccountMgr * self,
				       const gchar *name, 
				       const gchar *hostname,
				       guint portnumber,
				       const gchar *username, 
				       ImProtocolType proto,
				       ImProtocolType security,
				       ImProtocolType auth)
{
	ImAccountMgrPrivate *priv;
	const gchar *key;
	gboolean ok = TRUE;
	GError *err = NULL;
	ImProtocolRegistry *protocol_registry;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (strchr(name, '/') == NULL, FALSE);

	protocol_registry = im_protocol_registry_get_instance ();
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	/* hostname */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_HOSTNAME, TRUE);
	if (im_conf_key_exists (priv->im_conf, key, &err)) {
		g_printerr (_("im: server account '%s' already exists\n"), name);
		ok =  FALSE;
	}
	if (!ok)
		goto cleanup;
	
	im_conf_set_string (priv->im_conf, key, null_means_empty(hostname), &err);
	if (err) {
		g_printerr (_("im: failed to set %s: %s\n"), key, err->message);
		g_error_free (err);
		ok = FALSE;
	}
	if (!ok)
		goto cleanup;
	
	/* username */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_USERNAME, TRUE);
	ok = im_conf_set_string (priv->im_conf, key, null_means_empty (username), &err);
	if (err) {
		g_printerr (_("im: failed to set %s: %s\n"), key, err->message);
		g_error_free (err);
		ok = FALSE;
	}
	if (!ok)
		goto cleanup;
	
	
	/* proto */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_PROTO, TRUE);
	ok = im_conf_set_string (priv->im_conf, key,
				     im_protocol_get_name (im_protocol_registry_get_protocol_by_type (protocol_registry, proto)),
				     &err);
	if (err) {
		g_printerr (_("im: failed to set %s: %s\n"), key, err->message);
		g_error_free (err);
		ok = FALSE;
	}
	if (!ok)
		goto cleanup;


	/* portnumber */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_PORT, TRUE);
	ok = im_conf_set_int (priv->im_conf, key, portnumber, &err);
	if (err) {
		g_printerr (_("im: failed to set %s: %s\n"), key, err->message);
		g_error_free (err);
		ok = FALSE;
	}
	if (!ok)
		goto cleanup;

	
	/* auth mechanism */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_AUTH_MECH, TRUE);
	ok = im_conf_set_string (priv->im_conf, key,
				     im_protocol_get_name (im_protocol_registry_get_protocol_by_type (protocol_registry, auth)),
				     &err);
	if (err) {
		g_printerr (_("im: failed to set %s: %s\n"), key, err->message);
		g_error_free (err);
		ok = FALSE;
	}
	if (!ok)
		goto cleanup;
	
	/* Add the security settings: */
	im_account_mgr_set_server_account_security (self, name, security);

cleanup:
	if (!ok) {
		g_printerr (_("im: failed to add server account\n"));
		return FALSE;
	}

	return TRUE;
}

/** im_account_mgr_add_server_account_uri:
 * Only used for mbox and maildir accounts.
 */
gboolean
im_account_mgr_add_server_account_uri (ImAccountMgr * self,
					   const gchar *name, 
					   ImProtocolType proto,
					   const gchar *uri)
{
	ImAccountMgrPrivate *priv;
	const gchar *key;
	gboolean ok;
	ImProtocolRegistry *protocol_registry;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (strchr(name, '/') == NULL, FALSE);
	g_return_val_if_fail (uri, FALSE);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	protocol_registry = im_protocol_registry_get_instance ();
	
	/* proto */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_PROTO, TRUE);
	ok = im_conf_set_string (priv->im_conf, key,
				     im_protocol_get_name (im_protocol_registry_get_protocol_by_type (protocol_registry, proto)),
				     NULL);

	if (!ok) {
		g_printerr (_("im: failed to set proto\n"));
		return FALSE;
	}
	
	/* uri */
	key = _im_account_mgr_get_account_keyname_cached (priv, name, IM_ACCOUNT_URI, TRUE);
	ok = im_conf_set_string (priv->im_conf, key, uri, NULL);

	if (!ok) {
		g_printerr (_("im: failed to set uri\n"));
		return FALSE;
	}
	return TRUE;
}

/* 
 * Utility function used by im_account_mgr_remove_account
 */
static void
real_remove_account (ImConf *conf,
		     const gchar *acc_name,
		     gboolean server_account)
{
	GError *err = NULL;
	gchar *key;
	
	key = _im_account_mgr_get_account_keyname (acc_name, NULL, server_account);
	im_conf_remove_key (conf, key, &err);

	if (err) {
		g_printerr (_("im: error removing key: %s\n"), err->message);
		g_error_free (err);
	}
	g_free (key);
}

gboolean
im_account_mgr_remove_account (ImAccountMgr * self,
				   const gchar* name)
{
	ImAccountMgrPrivate *priv;
	gchar *default_account_name, *store_acc_name, *transport_acc_name;
	gboolean default_account_deleted;
	GSList *acc_names;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);

	if (!im_account_mgr_account_exists (self, name, FALSE)) {
		g_printerr (_("im: %s: account '%s' does not exist\n"), __FUNCTION__, name);
		return FALSE;
	}

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	default_account_deleted = FALSE;

	/* If this was the default, then remove that setting: */
	default_account_name = im_account_mgr_get_default_account (self);
	if (default_account_name && (strcmp (default_account_name, name) == 0)) {
		im_account_mgr_unset_default_account (self);
		default_account_deleted = TRUE;
	}
	g_free (default_account_name);

	/* Delete transport and store accounts */
	store_acc_name = im_account_mgr_get_string (self, name, 
							IM_ACCOUNT_STORE_ACCOUNT, FALSE);
	if (store_acc_name)
		real_remove_account (priv->im_conf, store_acc_name, TRUE);

	transport_acc_name = im_account_mgr_get_string (self, name, 
							    IM_ACCOUNT_TRANSPORT_ACCOUNT, FALSE);
	if (transport_acc_name)
		real_remove_account (priv->im_conf, transport_acc_name, TRUE);
			
	/* Remove the im account */
	real_remove_account (priv->im_conf, name, FALSE);

	if (default_account_deleted) {	
		/* pick another one as the new default account. We do
		   this *after* deleting the keys, because otherwise a
		   call to account_names will retrieve also the
		   deleted account */
		im_account_mgr_set_first_account_as_default (self);
	}

	/* if this was the last account, stop any auto-updating */
	/* (re)set the automatic account update */
	acc_names = im_account_mgr_get_account_ids (self, TRUE);
	if (!acc_names) {
#if 0
		im_platform_set_update_interval (0);
#endif
		/* it was the last account, the has_account / has_enabled_account
		 * changes
		 */
		priv->has_accounts = priv->has_enabled_accounts = FALSE; 
	} else
		im_account_mgr_free_account_ids (acc_names);
	
	/* Notify the observers. We do this *after* deleting
	   the keys, because otherwise a call to account_names
	   will retrieve also the deleted account */
	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_REMOVED_SIGNAL], 0, name);
	
	return TRUE;
}

gboolean
im_account_mgr_remove_server_account (ImAccountMgr * self,
					  const gchar* name)
{
	ImAccountMgrPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);

	if (!im_account_mgr_account_exists (self, name, TRUE)) {
		g_printerr (_("im: %s: server account '%s' does not exist\n"), __FUNCTION__, name);
		return FALSE;
	}

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	real_remove_account (priv->im_conf, name, TRUE);

	return TRUE;
}



/* strip the first /n/ character from each element
 * caller must make sure all elements are strings with
 * length >= n, and also that data can be freed.
 * change is in-place
 */
static void
strip_prefix_from_elements (GSList * lst, guint n)
{
	while (lst) {
		char *data;
		data = (char *) lst->data;
		memmove (data, data + n,
			 strlen(data) - n + 1);
		lst = lst->next;
	}
}


GSList*
im_account_mgr_get_account_ids (ImAccountMgr * self, gboolean only_enabled)
{
	GSList *accounts;
	ImAccountMgrPrivate *priv;
	GError *err = NULL;
	GSList *result = NULL;
	GSList *iter;
	
	/* we add 1 for the trailing "/" */
	const size_t prefix_len = strlen (IM_ACCOUNT_NAMESPACE) + 1;

	g_return_val_if_fail (self, NULL);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	accounts = im_conf_list_subkeys (priv->im_conf,
                                             IM_ACCOUNT_NAMESPACE, &err);

	if (err) {
		g_printerr (_("im: failed to get subkeys (%s): %s\n"),
			    IM_ACCOUNT_NAMESPACE, err->message);
		g_error_free (err);
		return NULL; /* assume accounts did not get value when err is set...*/
	}
	
	strip_prefix_from_elements (accounts, prefix_len);
		
	/* Unescape the keys to get the account names: */
	iter = accounts;
	while (iter) {
		const gchar* account_id_key;
		gchar* unescaped_name;
		gboolean add;

		if (!(iter->data)) {
			iter = iter->next;
			continue;
		}

		account_id_key = (const gchar*)iter->data;
		unescaped_name = account_id_key ? 
			im_conf_key_unescape (account_id_key) 
			: NULL;
		
		add = TRUE;
		if (only_enabled) {
			if (unescaped_name && 
			    !im_account_mgr_get_bool (self, unescaped_name, 
							  IM_ACCOUNT_ENABLED, FALSE))
				add = FALSE;
		}
		
		/* Ignore im accounts whose server accounts don't exist: 
		 * (We could be getting this list while the account is being deleted, 
		 * while the child server accounts have already been deleted, but the 
		 * parent im account already exists.
		 */
		if (add) {
			gchar* server_account_name = im_account_mgr_get_string
				(self, account_id_key, IM_ACCOUNT_STORE_ACCOUNT,
				 FALSE);
			if (server_account_name) {
				if (!im_account_mgr_account_exists (self, server_account_name, TRUE))
					add = FALSE;
				g_free (server_account_name);
			}
		}
		
		if (add) {
			gchar* server_account_name = im_account_mgr_get_string
				(self, account_id_key, IM_ACCOUNT_TRANSPORT_ACCOUNT,
				 FALSE);
			if (server_account_name) {
				if (!im_account_mgr_account_exists (self, server_account_name, TRUE))
					add = FALSE;
				g_free (server_account_name);
			}
		}
		
		if (add)
			result = g_slist_append (result, unescaped_name);
		else 
			g_free (unescaped_name);

		g_free (iter->data);
		iter->data = NULL;
		
		iter = g_slist_next (iter);	
	}
	

	/* we already freed the strings in the loop */
	g_slist_free (accounts);
	
	accounts = NULL;
	return result;
}


void
im_account_mgr_free_account_ids (GSList *account_ids)
{
	g_slist_foreach (account_ids, (GFunc)g_free, NULL);
	g_slist_free (account_ids);
}



gchar *
im_account_mgr_get_string (ImAccountMgr *self, const gchar *name,
			       const gchar *key, gboolean server_account) {

	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gchar *retval;
	GError *err = NULL;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	
	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = im_conf_get_string (priv->im_conf, keyname, &err);	
	if (err) {
		g_printerr (_("im: error getting string '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = NULL;
	}

	return retval;
}


gint
im_account_mgr_get_int (ImAccountMgr *self, const gchar *name, const gchar *key,
			    gboolean server_account)
{
	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gint retval;
	GError *err = NULL;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), -1);
	g_return_val_if_fail (name, -1);
	g_return_val_if_fail (key, -1);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	retval = im_conf_get_int (priv->im_conf, keyname, &err);
	if (err) {
		g_printerr (_("im: error getting int '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = -1;
	}

	return retval;
}



gboolean
im_account_mgr_get_bool (ImAccountMgr * self, const gchar *account,
			     const gchar * key, gboolean server_account)
{
	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (account, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	///keyname = _im_account_mgr_get_account_keyname (account, key, server_account);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, account, key, server_account);
		
	retval = im_conf_get_bool (priv->im_conf, keyname, &err);		
	if (err) {
		g_printerr (_("im: error getting bool '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = FALSE;
	}

	return retval;
}



GSList * 
im_account_mgr_get_list (ImAccountMgr *self, const gchar *name,
			     const gchar *key, ImConfValueType list_type,
			     gboolean server_account)
{
	ImAccountMgrPrivate *priv = NULL;

	const gchar *keyname;
	GSList *retval;
	GError *err = NULL;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key,
								  server_account);
	
	retval = im_conf_get_list (priv->im_conf, keyname, list_type, &err);
	if (err) {
		g_printerr (_("im: error getting list '%s': %s\n"), keyname,
			    err->message);
		g_error_free (err);
		retval = NULL;
	}
	return retval;
}


gboolean
im_account_mgr_set_string (ImAccountMgr * self, 
			       const gchar * name,
			       const gchar * key, 
			       const gchar * val, 
			       gboolean server_account)
{
	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	retval = im_conf_set_string (priv->im_conf, keyname, val, &err);
	if (err) {
		g_printerr (_("im: error setting string '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = FALSE;
	}
	return retval;
}

gboolean
im_account_mgr_set_int (ImAccountMgr * self, const gchar * name,
			    const gchar * key, int val, gboolean server_account)
{
	ImAccountMgrPrivate *priv;
	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	retval = im_conf_set_int (priv->im_conf, keyname, val, &err);
	if (err) {
		g_printerr (_("im: error setting int '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = FALSE;
	} else {
		/* check whether this field is one of those interesting for the 
		 * "account-updated" signal */
		if (strcmp(key, IM_ACCOUNT_LAST_UPDATED) == 0) {
			g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATED_SIGNAL], 
					0, name);
		}
	}
	return retval;
}



gboolean
im_account_mgr_set_bool (ImAccountMgr * self, const gchar * name,
			     const gchar * key, gboolean val, gboolean server_account)
{
	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	retval = im_conf_set_bool (priv->im_conf, keyname, val, &err);
	if (err) {
		g_printerr (_("im: error setting bool '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = FALSE;
	} else {
		/* check whether this field is one of those interesting for the 
		 * "account-updated" signal */
		if (strcmp (key, IM_ACCOUNT_HAS_NEW_MAILS) == 0) {
			g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATED_SIGNAL], 
					0, name);
		}
	}

	return retval;
}


gboolean
im_account_mgr_set_list (ImAccountMgr *self,
			     const gchar *name,
			     const gchar *key,
			     GSList *val,
			     ImConfValueType list_type,
			     gboolean server_account)
{
	ImAccountMgrPrivate *priv;
	const gchar *keyname;
	GError *err = NULL;
	gboolean retval;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key,  FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);

	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);
	
	retval = im_conf_set_list (priv->im_conf, keyname, val, list_type, &err);
	if (err) {
		g_printerr (_("im: error setting list '%s': %s\n"), keyname, err->message);
		g_error_free (err);
		retval = FALSE;
	}

	return retval;
}

gboolean
im_account_mgr_account_exists (ImAccountMgr * self, const gchar* name,
				   gboolean server_account)
{
	ImAccountMgrPrivate *priv;

	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;

	g_return_val_if_fail (self, FALSE);
        g_return_val_if_fail (name, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, NULL, server_account);
	retval = im_conf_key_exists (priv->im_conf, keyname, &err);
	if (err) {
		g_printerr (_("im: error determining existance of '%s': %s\n"), keyname,
			    err->message);
		g_error_free (err);
		retval = FALSE;
	}
	return retval;
}

gboolean
im_account_mgr_account_with_display_name_exists  (ImAccountMgr *self, 
						      const gchar *display_name)
{
	GSList *account_ids = NULL;
	GSList *cursor = NULL;
	gboolean found = FALSE;
	
	
	cursor = account_ids = im_account_mgr_get_account_ids (self, 
							       TRUE /* enabled accounts, because disabled accounts are not user visible. */);

	/* Look at each non-server account to check their display names; */
	while (cursor) {
		const gchar *account_id = (gchar*)cursor->data;
		const gchar *cursor_display_name;
		
		ImAccountSettings *settings = im_account_mgr_load_account_settings (self, account_id);
		if (!settings) {
			g_printerr (_("im: failed to get account data for %s\n"), account_id);
			cursor = cursor->next;
			continue;
		}

		cursor_display_name = im_account_settings_get_display_name (settings);
		if(cursor_display_name && (strcmp (cursor_display_name, display_name) == 0)) {
			found = TRUE;
			g_object_unref (settings);
			break;
		}

		g_object_unref (settings);
		cursor = cursor->next;
	}
	im_account_mgr_free_account_ids (account_ids);
	account_ids = NULL;
	
	return found;
}

static gboolean
server_accounts_equal (ImServerAccountSettings *s1,
		       ImServerAccountSettings *s2)
{
	const gchar *str1, *str2;

	if (im_server_account_settings_get_protocol (s1) !=
	    im_server_account_settings_get_protocol (s2))
		return FALSE;

	str1 = im_server_account_settings_get_username (s1);
	str2 = im_server_account_settings_get_username (s2);
	if (str1 && str2 && (str1 != str2) &&
	    strcmp (str1, str2) != 0)
		return FALSE;

	str1 = im_server_account_settings_get_hostname (s1);
	str2 = im_server_account_settings_get_hostname (s2);
	if (str1 && str2 && (str1 != str2) &&
	    strcmp (str1, str2) != 0)
		return FALSE;

	if (im_server_account_settings_get_port (s1) !=
	    im_server_account_settings_get_port (s2))
		return FALSE;

	return TRUE;
}

gboolean
im_account_mgr_check_already_configured_account  (ImAccountMgr *self, 
						      ImAccountSettings *settings)
{
	GSList *account_ids = NULL;
	GSList *cursor = NULL;
	ImServerAccountSettings *server_settings;
	gboolean found = FALSE;

	cursor = account_ids = im_account_mgr_get_account_ids (self, 
							       TRUE /* enabled accounts, because disabled accounts are not user visible. */);

	server_settings = im_account_settings_get_store_settings (settings);
	if (!server_settings) {
		g_printerr (_("im: couldn't get store settings from settings"));
		im_account_mgr_free_account_ids (account_ids);
		return FALSE;
	}
	
	/* Look at each non-server account to check their display names; */
	while (cursor && !found) {
		const gchar *account_id;
		ImAccountSettings *from_mgr_settings;
		ImServerAccountSettings *from_mgr_server_settings;

		account_id = (gchar*)cursor->data;		
		from_mgr_settings = im_account_mgr_load_account_settings (self, account_id);
		if (!settings) {
			g_printerr (_("im: failed to get account data for %s\n"), account_id);
			cursor = cursor->next;
			continue;
		}

		from_mgr_server_settings = im_account_settings_get_store_settings (from_mgr_settings);
		if (server_settings) {
			if (server_accounts_equal (from_mgr_server_settings, server_settings)) {
				found = TRUE;
			}
			g_object_unref (from_mgr_server_settings);
		} else {
			g_printerr (_("im: couldn't get store settings from account %s"), account_id);
		}
		g_object_unref (from_mgr_settings);
		cursor = cursor->next;
	}

	g_object_unref (server_settings);
	im_account_mgr_free_account_ids (account_ids);
	account_ids = NULL;
	
	return found;
}




gboolean 
im_account_mgr_unset (ImAccountMgr *self, const gchar *name,
			  const gchar *key, gboolean server_account)
{
	ImAccountMgrPrivate *priv;
	
	const gchar *keyname;
	gboolean retval;
	GError *err = NULL;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
        g_return_val_if_fail (name, FALSE);
        g_return_val_if_fail (key, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	keyname = _im_account_mgr_get_account_keyname_cached (priv, name, key, server_account);

	retval = im_conf_remove_key (priv->im_conf, keyname, &err);
	if (err) {
		g_printerr (_("im: error unsetting'%s': %s\n"), keyname,
			    err->message);
		g_error_free (err);
		retval = FALSE;
	}
	return retval;
}

gchar*
_im_account_mgr_account_from_key (const gchar *key, gboolean *is_account_key, gboolean *is_server_account)
{
	const gchar* account_ns;
	const gchar* server_account_ns;
	gchar *account = NULL;
	gchar *unescaped_name = NULL;

	/* Initialize input parameters: */
	if (is_account_key)
		*is_account_key = FALSE;

	if (is_server_account)
		*is_server_account = FALSE;

	account_ns        = im_defs_namespace (IM_ACCOUNT_SUBNAMESPACE "/");
	server_account_ns = im_defs_namespace (IM_SERVER_ACCOUNT_SUBNAMESPACE "/");

	/* determine whether it's an account or a server account,
	 * based on the prefix */
	if (g_str_has_prefix (key, account_ns)) {

		if (is_server_account)
			*is_server_account = FALSE;

		account = g_strdup (key + strlen (account_ns));

	} else if (g_str_has_prefix (key, server_account_ns)) {

		if (is_server_account)
			*is_server_account = TRUE;

		account = g_strdup (key + strlen (server_account_ns));
	} else
		return NULL;

	if (account) {
		gchar *cursor;

		/* if there are any slashes left in the key, it's not
		 * the toplevel entry for an account
		 */
		cursor = strstr(account, "/");

		if (cursor) {
			if (is_account_key)
				*is_account_key = TRUE;

			/* put a NULL where the first slash was */
			*cursor = '\0';
		}

		/* The key is an escaped string, so unescape it to get the actual account name */
		unescaped_name = im_conf_key_unescape (account);
		g_free (account);
	}

	return unescaped_name;
}





/* optimization: only with non-alphanum chars, escaping is needed */
inline static gboolean
is_alphanum (const gchar* str)
{
	const gchar *cursor;
	for (cursor = str; cursor && *cursor; ++cursor) {
		const char c = *cursor;
		/* we cannot trust isalnum(3), because it might consider locales */
		/*       numbers            ALPHA            alpha       */
		if (!((c>=48 && c<=57)||(c>=65 && c<=90)||(c>=97 && c<=122)))
			return FALSE;
	}
	return TRUE;
}
		



/* must be freed by caller */
gchar *
_im_account_mgr_get_account_keyname (const gchar *account_name, const gchar* name,
					 gboolean server_account)
{
	gchar *retval = NULL;	
	gchar *namespace = server_account ? (gchar *) IM_SERVER_ACCOUNT_NAMESPACE : (gchar *) IM_ACCOUNT_NAMESPACE;
	gchar *escaped_account_name, *escaped_name;
	
	if (!account_name)
		return g_strdup (namespace);

	/* optimization: only escape names when need to be escaped */
	if (is_alphanum (account_name))
		escaped_account_name = (gchar*)account_name;
	else
		escaped_account_name = im_conf_key_escape (account_name);
	
	if (is_alphanum (name))
		escaped_name = (gchar*)name;
	else
		escaped_name = im_conf_key_escape (name);
	//////////////////////////////////////////////////////////////

	if (escaped_account_name && escaped_name)
		retval = g_strconcat (namespace, "/", escaped_account_name, "/", escaped_name, NULL);
	else if (escaped_account_name)
		retval = g_strconcat (namespace, "/", escaped_account_name, NULL);

	/* Sanity check: */
	if (!retval || !im_conf_key_is_valid (retval)) {
		g_warning (_("%s: Generated conf key was invalid: %s"), __FUNCTION__,
			   retval ? retval: _("<empty>"));
		g_free (retval);
		retval = NULL;
	}
	
	/* g_free is only needed if we actually allocated anything */
	if (name != escaped_name)
		g_free (escaped_name);
	if (account_name != escaped_account_name)
		g_free (escaped_account_name);

	return retval;
}

static const gchar *
_im_account_mgr_get_account_keyname_cached (ImAccountMgrPrivate *priv, 
						const gchar* account_name,
						const gchar *name, 
						gboolean is_server)
{
	GHashTable *hash = is_server ? priv->server_account_key_hash : priv->account_key_hash;
	GHashTable *account_hash;
	gchar *key = NULL;
	const gchar *search_name;

	if (!account_name)
		return is_server ? IM_SERVER_ACCOUNT_NAMESPACE : IM_ACCOUNT_NAMESPACE;

	search_name = name ? name : "<dummy>";
	
	account_hash = g_hash_table_lookup (hash, account_name);	
	if (!account_hash) { /* no hash for this account yet? create it */
		account_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);		
		key = _im_account_mgr_get_account_keyname (account_name, name, is_server);
		g_hash_table_insert (account_hash, g_strdup(search_name), key);
		g_hash_table_insert (hash, g_strdup(account_name), account_hash);
		return key;
	}
	
	/* we have a hash for this account, but do we have the key? */	
	key = g_hash_table_lookup (account_hash, search_name);
	if (!key) {
		key = _im_account_mgr_get_account_keyname (account_name, name, is_server);
		g_hash_table_insert (account_hash, g_strdup(search_name), key);
	}
	
	return key;
}


gboolean
im_account_mgr_has_accounts (ImAccountMgr* self, gboolean enabled)
{
	ImAccountMgrPrivate* priv;
	GSList *account_ids;
	gboolean accounts_exist;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	
	if (enabled && priv->has_enabled_accounts)
		return TRUE;
	else if (!enabled && priv->has_accounts)
		return TRUE;
		
	/* Check that at least one account exists: */
	account_ids = im_account_mgr_get_account_ids (self,enabled);
	accounts_exist = account_ids != NULL;
	im_account_mgr_free_account_ids (account_ids);
	account_ids = NULL;

	/* cache it. */
	if (enabled)
		priv->has_enabled_accounts = accounts_exist;
	else
		priv->has_accounts = accounts_exist;
	
	return accounts_exist;
}

static int
compare_account_id(gconstpointer a, gconstpointer b)
{
	const gchar* account_id = (const gchar*) a;
	const gchar* account_id2 = (const gchar*) b;
	return strcmp(account_id, account_id2);
}

void 
im_account_mgr_set_account_busy(ImAccountMgr* self, 
				const gchar* account_id, 
				gboolean busy)
{
	ImAccountMgrPrivate* priv;

	g_return_if_fail (IM_IS_ACCOUNT_MGR(self));
	g_return_if_fail (account_id);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	if (busy) {
		GSList *account_ids = im_account_mgr_get_account_ids (self, TRUE);
		GSList* account = g_slist_find_custom(account_ids, account_id,
						      (GCompareFunc) compare_account_id);

		if (account && !im_account_mgr_account_is_busy(self, account_id)) {
			priv->busy_accounts = g_slist_append(priv->busy_accounts, g_strdup(account_id));
			g_signal_emit (G_OBJECT(self), signals[ACCOUNT_BUSY_SIGNAL], 
				       0, account_id, TRUE);
		}
		im_account_mgr_free_account_ids (account_ids);
		account_ids = NULL;
	} else {
		GSList* account = 
			g_slist_find_custom(priv->busy_accounts, account_id, (GCompareFunc) compare_account_id);

		if (account) {
			g_free(account->data);
			priv->busy_accounts = g_slist_delete_link(priv->busy_accounts, account);
			g_signal_emit (G_OBJECT(self), signals[ACCOUNT_BUSY_SIGNAL], 
				       0, account_id, FALSE);
		}
	}
}

gboolean
im_account_mgr_account_is_busy (ImAccountMgr* self, const gchar* account_id)
{
	ImAccountMgrPrivate* priv;
	
	g_return_val_if_fail (IM_IS_ACCOUNT_MGR(self), FALSE);
	g_return_val_if_fail (account_id, FALSE);

	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
		
	return (g_slist_find_custom(priv->busy_accounts, account_id, (GCompareFunc) compare_account_id)
		!= NULL);
}

void
im_account_mgr_notify_account_update (ImAccountMgr* self, 
					  const gchar *server_account_name)
{
	ImProtocolType proto;
	ImAccountMgrPrivate* priv;
	ImProtocolRegistry *protocol_registry;
	gchar *proto_name = NULL;

	g_return_if_fail (self);
	g_return_if_fail (server_account_name);
	
	priv = IM_ACCOUNT_MGR_GET_PRIVATE (self);
	protocol_registry = im_protocol_registry_get_instance ();
	
	/* Get protocol */
	proto_name = im_account_mgr_get_string (self, server_account_name, 
						    IM_ACCOUNT_PROTO, TRUE);
	if (!proto_name) {
		g_return_if_reached ();
		return;
	}
	proto = im_protocol_get_type_id (im_protocol_registry_get_protocol_by_name (protocol_registry,
											    IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
											    proto_name));
	g_free (proto_name);

	/* there is some update in the account, so we can't
	 * be sure about whether there are still enabled accounts...
	 */
	priv->has_enabled_accounts = FALSE;
	priv->has_accounts         = FALSE;
	
	/* Emit "update-account" */
	g_signal_emit (G_OBJECT(self), 
		       signals[ACCOUNT_CHANGED_SIGNAL], 0, 
		       server_account_name, 
		       (im_protocol_registry_protocol_type_has_tag (protocol_registry, proto, IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS)) ? 
		       IM_ACCOUNT_TYPE_STORE : 
		       IM_ACCOUNT_TYPE_TRANSPORT);
}


gboolean
im_account_mgr_set_default_account  (ImAccountMgr *self, const gchar* account)
{
	ImConf *conf;
	gboolean retval;
	
	g_return_val_if_fail (self,    FALSE);
	g_return_val_if_fail (account, FALSE);
	g_return_val_if_fail (im_account_mgr_account_exists (self, account, FALSE),
			      FALSE);
	
	conf = IM_ACCOUNT_MGR_GET_PRIVATE (self)->im_conf;

	/* Change the default account and notify */
	retval = im_conf_set_string (conf, IM_CONF_DEFAULT_ACCOUNT, account, NULL);
	if (retval)
		g_signal_emit (G_OBJECT(self), signals[DEFAULT_ACCOUNT_CHANGED_SIGNAL], 0);

	return retval;
}


gchar*
im_account_mgr_get_default_account  (ImAccountMgr *self)
{
	gchar *account;	
	ImConf *conf;
	GError *err = NULL;
	
	g_return_val_if_fail (self, NULL);

	conf = IM_ACCOUNT_MGR_GET_PRIVATE (self)->im_conf;
	account = im_conf_get_string (conf, IM_CONF_DEFAULT_ACCOUNT, &err);
	
	if (err) {
		g_printerr (_("im: failed to get '%s': %s\n"),
			    IM_CONF_DEFAULT_ACCOUNT, err->message);
		g_error_free (err);
		return  NULL;
	}
	
	/* sanity check */
	if (account && !im_account_mgr_account_exists (self, account, FALSE)) {
		g_printerr (_("im: default account does not exist\n"));
		g_free (account);
		return NULL;
	}

	return account;
}

static gboolean
im_account_mgr_unset_default_account (ImAccountMgr *self)
{
	ImConf *conf;
	gboolean retval;
	
	g_return_val_if_fail (self,    FALSE);

	conf = IM_ACCOUNT_MGR_GET_PRIVATE (self)->im_conf;
		
	retval = im_conf_remove_key (conf, IM_CONF_DEFAULT_ACCOUNT, NULL /* err */);

	if (retval)
		g_signal_emit (G_OBJECT(self), signals[DEFAULT_ACCOUNT_CHANGED_SIGNAL], 0);

	return retval;
}


gchar* 
im_account_mgr_get_display_name (ImAccountMgr *self, 
				     const gchar* name)
{
	return im_account_mgr_get_string (self, name, IM_ACCOUNT_DISPLAY_NAME, FALSE);
}

void 
im_account_mgr_set_display_name (ImAccountMgr *self, 
				     const gchar *account_name,
				     const gchar *display_name)
{
	gboolean notify = TRUE;

	if (!im_account_mgr_get_display_name (self, account_name))
		notify = FALSE;

	im_account_mgr_set_string (self, 
				       account_name,
				       IM_ACCOUNT_DISPLAY_NAME, 
				       display_name, 
				       FALSE /* not server account */);

	/* Notify about the change in the display name */
	if (notify)
		g_signal_emit (self, signals[DISPLAY_NAME_CHANGED_SIGNAL], 0, account_name);
}

gboolean 
im_account_mgr_singleton_protocol_exists (ImAccountMgr *mgr,
					      ImProtocolType protocol_type)
{
	GSList *account_ids, *node;
	gboolean found = FALSE;

	g_return_val_if_fail (IM_IS_ACCOUNT_MGR (mgr), FALSE);
	account_ids = im_account_mgr_get_account_ids (mgr, FALSE);

	for (node = account_ids; node != NULL; node = g_slist_next (node)) {
		ImProtocolType current_protocol;

		current_protocol = im_account_mgr_get_store_protocol (mgr, (gchar *) node->data);
		if (current_protocol == protocol_type) {
			found = TRUE;
			break;
		}
	}

	im_account_mgr_free_account_ids (account_ids);

	return found;
}
