/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-mgr-helpers.c : helpers methods for dealing with ImAccountMgr */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Alberto Garcia Gonzalez <agarcia@igalia.com>
 *  Philip Van Hoof
 *  Dirk-Jan C. Binnema
 *  Murray Cumming
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

#include <im-account-mgr-helpers.h>

#include <im-account-protocol.h>
#include <im-account-mgr-priv.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const gchar * null_means_empty (const gchar * str);

static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}

/**
 * im_account_mgr_set_enabled:
 * @self: an #ImAccountMgr
 * @name: the account name 
 * @enabled: if %TRUE, the account will be enabled, if %FALSE, it will be disabled
 * 
 * enable/disabled an account
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 */
gboolean
im_account_mgr_set_enabled (ImAccountMgr *self,
			    const gchar* name,
			    gboolean enabled)
{
	gboolean result;
	result = im_account_mgr_set_bool (self, name, IM_ACCOUNT_ENABLED, enabled,FALSE);
	return result;
}


/**
 * im_account_mgr_get_enabled:
 * @self: an #ImAccountMgr
 * @name: the account name
 *
 * check whether a certain account is enabled
 *
 * Returns: TRUE if it is enabled, FALSE otherwise
 */
gboolean
im_account_mgr_get_enabled (ImAccountMgr *self,
			    const gchar* name)
{
	return im_account_mgr_get_bool (self, name, IM_ACCOUNT_ENABLED, FALSE);
}

/**
 * im_account_mgr_set_signature:
 * @self: an #ImAccountMgr
 * @name: the account name
 * @signature: the signature text 
 * @use_signature: Whether the signature should be used.
 * 
 * Sets the signature text for the account.
 *
 * Returns: %TRUE if it worked, %FALSE otherwise
 */
gboolean
im_account_mgr_set_signature (ImAccountMgr *self, 
			      const gchar* name, 
			      const gchar* signature,
			      gboolean use_signature)
{
	gboolean result = im_account_mgr_set_bool (self, name, IM_ACCOUNT_USE_SIGNATURE, 
						   use_signature, FALSE);
	result = result && im_account_mgr_set_string (self, name, IM_ACCOUNT_SIGNATURE, 
						      null_means_empty (signature), FALSE);
	return result;
}

/**
 * im_account_mgr_get_signature:
 * @self: an #ImAccountMgr
 * @name: the account name
 * @use_signature: (out): #gboolean pointer set to TRUE if the signature should be used.
 *
 * Gets the signature text for this account.
 *
 * Returns: (transfer full): The signature text
 */
gchar* 
im_account_mgr_get_signature (ImAccountMgr *self, 
			      const gchar* name, 
			      gboolean* use_signature)
{
	*use_signature = 
		im_account_mgr_get_bool (self, name, IM_ACCOUNT_USE_SIGNATURE, FALSE);
	
	return im_account_mgr_get_string (self, name, IM_ACCOUNT_SIGNATURE, FALSE);
}

/**
 * im_account_mgr_get_store_protocol:
 * @self: an #ImAccountMgr
 * @name: the account name
 *
 * Gets the protocol type (For instance, POP or IMAP) used for the store server account.
 *
 * Returns: an #ImProtocolType
 */
ImProtocolType
im_account_mgr_get_store_protocol (ImAccountMgr *self, const gchar* name)
{
       ImProtocolType result = IM_PROTOCOLS_STORE_POP; /* Arbitrary default */
       
       gchar *server_account_name = im_account_mgr_get_string (self, name,
							       IM_ACCOUNT_STORE_ACCOUNT,
							       FALSE);
       if (server_account_name) {
	       ImServerAccountSettings* server_settings = 
		       im_account_mgr_load_server_settings (self, server_account_name, FALSE);
	       result = im_server_account_settings_get_protocol (server_settings);
	 
	       g_object_unref (server_settings);
	       g_free (server_account_name);
       }
       
       return result;
}

/**
 * im_account_mgr_get_server_account_username:
 * @self: an #ImAccountMgr
 * @account_name: The name of the server account.
 *
 * Gets the username this server account.
 *
 * Returns: (transfer full): The username.
 */
gchar*
im_account_mgr_get_server_account_username (ImAccountMgr *self,
					    const gchar* account_name)
{
	return im_account_mgr_get_string (self, account_name, IM_ACCOUNT_USERNAME, 
					  TRUE /* server account */);
}

/**
 * im_account_mgr_set_server_account_username:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 * @username: The new username.
 *
 * Sets the username this server account.
 */
void
im_account_mgr_set_server_account_username (ImAccountMgr *self,
					    const gchar* account_name, 
					    const gchar* username)
{
	/* Note that this won't work properly as long as the gconf cache is broken 
	 * in Maemo Bora: */
	gchar *existing_username = im_account_mgr_get_server_account_username(self, 
									      account_name);
	
	im_account_mgr_set_string (self, account_name, IM_ACCOUNT_USERNAME, 
				   username, TRUE /* server account */);
		
	/* We don't know anything about new usernames: */
	if (strcmp (existing_username, username) != 0)
		im_account_mgr_set_server_account_username_has_succeeded (self, account_name, FALSE);
	
	g_free (existing_username);
}

/**
 * im_account_mgr_get_server_account_username_has_succeeded:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 *
 * Whether a connection has ever been successfully made to this account with 
 * the current username. This can be used to avoid asking again for the username 
 * when asking a second time for a non-stored password.
 *
 * Returns: %TRUE if the username is known to be correct.
 */
gboolean
im_account_mgr_get_server_account_username_has_succeeded (ImAccountMgr *self,
							  const gchar* account_name)
{
	return im_account_mgr_get_bool (self, account_name, IM_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
					TRUE /* server account */);
}

/**
 * im_account_mgr_set_server_account_username_has_succeeded:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 * @succeeded: Whether the username has succeeded
 *
 * Sets whether the username is known to be correct.
 */
void
im_account_mgr_set_server_account_username_has_succeeded (ImAccountMgr *self, 
							  const gchar* account_name, 
							  gboolean succeeded)
{
	im_account_mgr_set_bool (self, account_name, IM_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
				 succeeded, TRUE /* server account */);
}

/**
 * im_server_account_im_account_mgr_get_server_account_hostname:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 *
 * Gets the hostname this server account.
 *
 * Returns: (transfer full): The hostname.
 */
gchar*
im_account_mgr_get_server_account_hostname (ImAccountMgr *self, 
					    const gchar* account_name)
{
	return im_account_mgr_get_string (self, 
					  account_name, 
					  IM_ACCOUNT_HOSTNAME, 
					  TRUE /* server account */);
}
 
/**
 * im_server_account_im_account_mgr_set_server_account_hostname:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 * @hostname: The new hostname
 *
 * Sets the hostname this server account.
 */
void
im_account_mgr_set_server_account_hostname (ImAccountMgr *self, 
					    const gchar *server_account_name,
					    const gchar *hostname)
{
	im_account_mgr_set_string (self, 
				   server_account_name,
				   IM_ACCOUNT_HOSTNAME, 
				   hostname, 
				   TRUE /* server account */);
}


/**
 * im_account_mgr_get_server_account_secure_auth:
 * @self: an #ImAccountMgr
 * @account_name: the name of the server account
 *
 * Obtains the authentication method for @account_name
 *
 * Returns: an #ImProtocolType
 */
ImProtocolType
im_account_mgr_get_server_account_secure_auth (ImAccountMgr *self, 
					       const gchar* account_name)
{
	ImProtocolRegistry *protocol_registry;
	ImProtocolType result = IM_PROTOCOLS_AUTH_NONE;
	gchar* value;

	protocol_registry = im_protocol_registry_get_instance ();
	value = im_account_mgr_get_string (self, account_name, IM_ACCOUNT_AUTH_MECH, 
					       TRUE /* server account */);
	if (value) {
		ImProtocol *protocol;

		protocol = im_protocol_registry_get_protocol_by_name (protocol_registry, IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS, value);
		g_free (value);

		if (protocol)
			result = im_protocol_get_type_id (protocol);
			
	}
	
	return result;
}


/**
 * im_account_mgr_get_server_account_secure_auth:
 * @self: an #ImAccountMgr
 * @account_name: the name of the server account
 * @secure_auth: an #ImProtocolType
 *
 * Sets the authentication method for @account_name
 */
void
im_account_mgr_set_server_account_secure_auth (ImAccountMgr *self, 
					       const gchar* account_name,
					       ImProtocolType secure_auth)
{
	const gchar* str_value;
	ImProtocolRegistry *protocol_registry;
	ImProtocol *protocol;

	/* Get the conf string for the protocol: */
	protocol_registry = im_protocol_registry_get_instance ();
	protocol = im_protocol_registry_get_protocol_by_type (protocol_registry, secure_auth);
	str_value = im_protocol_get_name (protocol);
	
	/* Set it in the configuration: */
	im_account_mgr_set_string (self, account_name, IM_ACCOUNT_AUTH_MECH, str_value, TRUE);
}

/**
 * im_server_account_data_get_server_account_security:
 * @self: an #ImAccountMgr
 * @account_name: The name of a server account.
 *
 * Gets the security protocol for this server account.
 *
 * Returns: an #ImProtocolType
 */
ImProtocolType
im_account_mgr_get_server_account_security (ImAccountMgr *self, 
					    const gchar* account_name)
{
	ImProtocolType result = IM_PROTOCOLS_CONNECTION_NONE;
	gchar* value;

	value = im_account_mgr_get_string (self, account_name, IM_ACCOUNT_SECURITY, 
					   TRUE /* server account */);
	if (value) {
		ImProtocolRegistry *protocol_registry;
		ImProtocol *protocol;
		
		protocol_registry = im_protocol_registry_get_instance ();
		protocol = im_protocol_registry_get_protocol_by_name (protocol_registry,
								      IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
								      value);
		g_free (value);
		
		if (protocol)
			result = im_protocol_get_type_id (protocol);
	}
	
	return result;
}


/**
 * im_account_mgr_set_server_account_security:
 * @self: an #ImAccountMgr
 * @account_name: the server account name
 * @secure_auth: an #ImProtocolType
 *
 * Sets the security protocol for this server account.
 */
void
im_account_mgr_set_server_account_security (ImAccountMgr *self, 
					    const gchar* account_name, 
					    ImProtocolType security)
{
	const gchar* str_value;
	ImProtocolRegistry *protocol_registry;
	ImProtocol *protocol;

	/* Get the conf string for the protocol type: */
	protocol_registry = im_protocol_registry_get_instance ();
	protocol = im_protocol_registry_get_protocol_by_type (protocol_registry, security);
	str_value = im_protocol_get_name (protocol);
	
	/* Set it in the configuration: */
	im_account_mgr_set_string (self, account_name, IM_ACCOUNT_SECURITY, str_value, TRUE);
}

/**
 * im_account_mgr_load_server_settings:
 * @self: an #ImAccountMgr
 * @name: the server account name
 * @is_transport_and_not_store: %TRUE if account is transport, %FALSE if it's store.
 *
 * Load @name server account settings.
 *
 * Returns: (transfer full): an #ImServerAccountSettings
 */
ImServerAccountSettings *
im_account_mgr_load_server_settings (ImAccountMgr *self,
				     const gchar* name,
				     gboolean is_transport_and_not_store)
{
	ImServerAccountSettings *settings = NULL;
	ImProtocol *protocol;
	ImProtocolRegistry *registry;
	gchar *hostname, *username, *uri, *proto, *auth, *sec;

	if (!im_account_mgr_account_exists (self, name, TRUE)) {
		g_warning ("%s account %s does not exist", __FUNCTION__, name);
		return NULL;
	}

	registry = im_protocol_registry_get_instance ();
	settings = im_server_account_settings_new ();

	im_server_account_settings_set_account_name (settings, name);

	proto = im_account_mgr_get_string (self, name, IM_ACCOUNT_PROTO, TRUE);
	if (proto) {
		gchar *tag = NULL;
		if (is_transport_and_not_store) {
			tag = IM_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS;
		} else {
			tag = IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS;
		}
		protocol = im_protocol_registry_get_protocol_by_name (registry, tag, proto);

		im_server_account_settings_set_protocol (settings,
							 im_protocol_get_type_id (protocol));
		g_free (proto);
	} else {
		goto on_error;
	}
	
	im_server_account_settings_set_port (settings,
					     im_account_mgr_get_int (self, name, IM_ACCOUNT_PORT, TRUE));

	auth = im_account_mgr_get_string (self, name, IM_ACCOUNT_AUTH_MECH, TRUE);
	if (auth) {
		protocol = im_protocol_registry_get_protocol_by_name (registry, IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS, auth);
		im_server_account_settings_set_auth_protocol (settings,
							      im_protocol_get_type_id (protocol));
		g_free (auth);
	} else {
		im_server_account_settings_set_auth_protocol (settings, IM_PROTOCOLS_AUTH_NONE);
	}
	
	sec = im_account_mgr_get_string (self, name, IM_ACCOUNT_SECURITY, TRUE);
	if (sec) {
		protocol = im_protocol_registry_get_protocol_by_name (registry, IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS, sec);
		im_server_account_settings_set_security_protocol (settings,
								  im_protocol_get_type_id (protocol));
		g_free (sec);
	} else {
		im_server_account_settings_set_security_protocol (settings,
								  IM_PROTOCOLS_CONNECTION_NONE);
	}
	
	/* Username and URI. Note that the URI could include
	   the former two, so in this case there is no need to have
	   them */
	username = im_account_mgr_get_string (self, name,
					      IM_ACCOUNT_USERNAME,TRUE);
	if (username)
		im_server_account_settings_set_username (settings, username);
	
	uri = im_account_mgr_get_string (self, name,
					 IM_ACCOUNT_URI, TRUE);
	if (uri)
		im_server_account_settings_set_uri (settings, uri);
	
	hostname = im_account_mgr_get_string (self, name,
					      IM_ACCOUNT_HOSTNAME,TRUE);
	if (hostname)
		im_server_account_settings_set_hostname (settings, hostname);
	
	if (!uri) {
		if (!username || !hostname) {
			g_free (username);
			g_free (hostname);
			goto on_error;
		}
	}
	
	g_free (username);
	g_free (hostname);
	g_free (uri);
	
	return settings;
	
 on_error:
	if (settings)
		g_object_unref (settings);
	return NULL;
}

/**
 * im_account_mgr_save_server_settings:
 * @self: an #ImAccountMgr instance
 * @settings: an #ImServerAccountSettings
 *
 * Saves the server account settings to persistent storage
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
gboolean 
im_account_mgr_save_server_settings (ImAccountMgr *self,
				     ImServerAccountSettings *settings)
{
	gboolean has_errors = FALSE;
	const gchar *account_name;
	const gchar *protocol_name;
	const gchar *uri;
	ImProtocolRegistry *protocol_registry;
	ImProtocol *protocol;
	
	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), FALSE);
	protocol_registry = im_protocol_registry_get_instance ();
	account_name = im_server_account_settings_get_account_name (settings);
	
	/* if we don't have a valid account name we cannot save */
	g_return_val_if_fail (account_name, FALSE);
	
	protocol = im_protocol_registry_get_protocol_by_type (protocol_registry,
							      im_server_account_settings_get_protocol (settings));
	protocol_name = im_protocol_get_name (protocol);
	uri = im_server_account_settings_get_uri (settings);
	if (!uri) {
		const gchar *hostname;
		const gchar *username;
		gint port;
		const gchar *auth_protocol_name;
		const gchar *security_name;

		hostname = null_means_empty (im_server_account_settings_get_hostname (settings));
		username = null_means_empty (im_server_account_settings_get_username (settings));
		port = im_server_account_settings_get_port (settings);
		protocol = im_protocol_registry_get_protocol_by_type (protocol_registry,
								      im_server_account_settings_get_auth_protocol (settings));
		auth_protocol_name = im_protocol_get_name (protocol);
		protocol = im_protocol_registry_get_protocol_by_type (protocol_registry,
								      im_server_account_settings_get_security_protocol (settings));
		security_name = im_protocol_get_name (protocol);
		
		has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_HOSTNAME, 
							 hostname, TRUE);
		if (!has_errors)
			(has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_USERNAME,
								  username, TRUE));
		if (!has_errors)
			(has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_PROTO,
								  protocol_name, TRUE));
		if (!has_errors)
			(has_errors = !im_account_mgr_set_int (self, account_name, IM_ACCOUNT_PORT,
							       port, TRUE));
		if (!has_errors)
			(has_errors = !im_account_mgr_set_string (self, account_name, 
								  IM_ACCOUNT_AUTH_MECH,
								  auth_protocol_name, TRUE));		
		if (!has_errors)
			(has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_SECURITY,
								  security_name,
								  TRUE));
	} else {
		const gchar *uri = im_server_account_settings_get_uri (settings);
		has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_URI,
							 uri, TRUE);
		if (!has_errors)
			(has_errors = !im_account_mgr_set_string (self, account_name, IM_ACCOUNT_PROTO,
								  protocol_name, TRUE));
	}
	
	return !has_errors;
	
}


/**
 * im_account_mgr_load_account_settings:
 * @self: an #ImAccountMgr
 * @name: the name of the account
 * 
 * get information about an account
 *
 * Returns: (transfer full): an #ImAccountSettings, or %NULL if account is not valid
 * or does not exist.
 */
ImAccountSettings *
im_account_mgr_load_account_settings (ImAccountMgr *self, 
				      const gchar *name)
{
	ImAccountSettings *settings;
	gchar *string;
	gchar *server_account;
	gchar *default_account;
	gboolean use_signature = FALSE;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	
	if (!im_account_mgr_account_exists (self, name, FALSE)) {
		/* For instance, maybe you are mistakenly checking for a server account name? */
		g_warning ("%s: Account %s does not exist.", __FUNCTION__, name);
		return NULL;
	}
	
	settings = im_account_settings_new ();

	im_account_settings_set_account_name (settings, name);

	string = im_account_mgr_get_string (self, name,
						IM_ACCOUNT_DISPLAY_NAME,
						FALSE);
	im_account_settings_set_display_name (settings, string);
	g_free (string);

 	string = im_account_mgr_get_string (self, name,
						IM_ACCOUNT_FULLNAME,
						FALSE);
	im_account_settings_set_fullname (settings, string);
	g_free (string);

	string = im_account_mgr_get_string (self, name,
						IM_ACCOUNT_EMAIL,
						FALSE);
	im_account_settings_set_email_address (settings, string);
	g_free (string);

	im_account_settings_set_enabled (settings, im_account_mgr_get_enabled (self, name));
	im_account_settings_set_retrieve_type (settings, im_account_mgr_get_retrieve_type (self, name));
	im_account_settings_set_retrieve_limit (settings, im_account_mgr_get_retrieve_limit (self, name));

	default_account    = im_account_mgr_get_default_account (self);
	im_account_settings_set_is_default (settings,
						(default_account && strcmp (default_account, name) == 0));
	g_free (default_account);

	string = im_account_mgr_get_signature (self, name, &use_signature);
	im_account_settings_set_use_signature (settings, use_signature);
	im_account_settings_set_signature (settings, string);
	g_free (string);

	im_account_settings_set_leave_messages_on_server 
		(settings, im_account_mgr_get_leave_on_server (self, name));

	/* store */
	server_account     = im_account_mgr_get_string (self, name,
							    IM_ACCOUNT_STORE_ACCOUNT,
							    FALSE);
	if (server_account) {
		ImServerAccountSettings *store_settings;
		store_settings = im_account_mgr_load_server_settings (self, server_account, FALSE);
		g_free (server_account);

		/* It could happen that the account data is corrupted
		   so it's not loaded properly */
		if (store_settings) {
			im_account_settings_set_store_settings (settings,
								    store_settings);
			g_object_unref (store_settings);
		} else {
			g_warning ("%s can not load server settings. Account corrupted?", __FUNCTION__);
			g_object_unref (settings);
			return NULL;
		}
	}

	/* transport */
	server_account = im_account_mgr_get_string (self, name,
							IM_ACCOUNT_TRANSPORT_ACCOUNT,
							FALSE);
	if (server_account) {
		ImServerAccountSettings *transport_settings;
		transport_settings = im_account_mgr_load_server_settings (self, server_account, TRUE);
		g_free (server_account);

		if (transport_settings) {
			im_account_settings_set_transport_settings (settings, transport_settings);
			g_object_unref (transport_settings);
		} else {
			g_warning ("%s can not load server settings. Account corrupted?", __FUNCTION__);
			g_object_unref (settings);
			return NULL;
		}
	}

	return settings;
}

/**
 * im_account_mgr_save_account_settings:
 * @self: an #ImAccountMgr
 * @settings: an #ImAccountSettings
 * 
 * Saves to persistent storage @settings
 */
void
im_account_mgr_save_account_settings (ImAccountMgr *mgr,
				      ImAccountSettings *settings)
{
	const gchar *account_name;
	ImServerAccountSettings *store_settings;
	ImServerAccountSettings *transport_settings;

	g_return_if_fail (IM_IS_ACCOUNT_MGR (mgr));
	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	account_name = im_account_settings_get_account_name (settings);
	g_return_if_fail (account_name != NULL);

	im_account_mgr_set_display_name (mgr, account_name,
					     im_account_settings_get_display_name (settings));
	im_account_mgr_set_user_fullname (mgr, account_name,
					      im_account_settings_get_fullname (settings));
	im_account_mgr_set_user_email (mgr, account_name,
					   im_account_settings_get_email_address (settings));
	im_account_mgr_set_retrieve_type (mgr, account_name,
					      im_account_settings_get_retrieve_type (settings));
	im_account_mgr_set_retrieve_limit (mgr, account_name,
					       im_account_settings_get_retrieve_limit (settings));
	im_account_mgr_set_leave_on_server (mgr, account_name,
						im_account_settings_get_leave_messages_on_server (settings));
	im_account_mgr_set_signature (mgr, account_name,
					  im_account_settings_get_signature (settings),
					  im_account_settings_get_use_signature (settings));
	store_settings = im_account_settings_get_store_settings (settings);
	if (store_settings) {
		const gchar *store_account_name;
		store_account_name = im_server_account_settings_get_account_name (store_settings);
		if (store_account_name)
			im_account_mgr_set_string (mgr, account_name, IM_ACCOUNT_STORE_ACCOUNT, 
						       store_account_name, FALSE);
		im_account_mgr_save_server_settings (mgr, store_settings);
		g_object_unref (store_settings);
	}

	transport_settings = im_account_settings_get_transport_settings (settings);
	if (transport_settings) {
		const gchar *transport_account_name;
		transport_account_name = im_server_account_settings_get_account_name (transport_settings);
		if (transport_account_name)
			im_account_mgr_set_string (mgr, account_name, IM_ACCOUNT_TRANSPORT_ACCOUNT, 
						       transport_account_name, FALSE);
		im_account_mgr_save_server_settings (mgr, transport_settings);
		g_object_unref (transport_settings);
	}
	im_account_mgr_set_bool (mgr, account_name, IM_ACCOUNT_ENABLED, TRUE,FALSE);
}


static gint 
on_accounts_list_sort_by_title(gconstpointer a, gconstpointer b)
{
 	return g_utf8_collate((const gchar*)a, (const gchar*)b);
}

/**
 * im_account_mgr_get_first_account_name:
 * @self: an #ImAccountMgr
 *
 * Get the first one, alphabetically, by title.
 *
 * Returns: (transfer full): the name of the first account
 */
gchar* 
im_account_mgr_get_first_account_name (ImAccountMgr *self)
{
	const gchar* account_name = NULL;
	GSList* list_sorted;
	GSList *iter;
	gboolean found = FALSE;
	GSList *account_names;
	gchar* result = NULL;
	
	account_names = im_account_mgr_account_names (self, TRUE /* only enabled */);

	/* Return TRUE if there is no account */
	if (!account_names)
		return NULL;

	/* Get the first one, alphabetically, by title: */
	/* gchar *old_default = im_account_mgr_get_default_account (self); */
	list_sorted = g_slist_sort (account_names, on_accounts_list_sort_by_title);

	iter = list_sorted;
	while (iter && !found) {
		account_name = (const gchar*)list_sorted->data;

		if (account_name)
			found = TRUE;

		if (!found)
			iter = g_slist_next (iter);
	}

	if (account_name)
		result = g_strdup (account_name);
		
	im_account_mgr_free_account_names (account_names);
	account_names = NULL;

	return result;
}

/**
 * im_account_mgr_set_first_account_as_default:
 * @self: an #ImAccountMgr
 * 
 * Guarantees that at least one account, if there are any accounts, is the default,
 * so that im_account_mgr_get_default_account() will return non-NULL if there 
 * are any accounts.
 *
 * Returns: %TRUE if succeeded, %FALSE otherwise
 */
gboolean
im_account_mgr_set_first_account_as_default (ImAccountMgr *self)
{
	gboolean result = FALSE;
	gchar* account_name;

	account_name = im_account_mgr_get_first_account_name(self);
	if (account_name) {
		result = im_account_mgr_set_default_account (self, account_name);
		g_free (account_name);
	}
	else
		result = TRUE; /* If there are no accounts then it's not a failure. */

	return result;
}

/**
 * im_account_mgr_get_from_string:
 * @self: a #ImAccountMgr
 * @name: the account name
 * @mailbox: the mailbox
 *
 * get the From: string for some account; ie. "Foo Bar" &lt;foo.bar@cuux.yy&gt;"
 *
 * Returns: (transfer full): the from-string, or %NULL in case of error
 */
gchar*
im_account_mgr_get_from_string (ImAccountMgr *self,
				const gchar* name,
				const gchar *mailbox)
{
	gchar *from;
	gchar *fullname, *email;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	
	fullname      = im_account_mgr_get_string (self, name, IM_ACCOUNT_FULLNAME,
						   FALSE);
	email         = im_account_mgr_get_string (self, name, IM_ACCOUNT_EMAIL,
						   FALSE);
	from = g_strdup_printf ("%s <%s>",
				fullname ? fullname : "",
				email    ? email    : "");
	g_free (fullname);
	g_free (email);
	return from;
}

/* Add a number to the end of the text, or increment a number that is already there.
 */
static gchar*
util_increment_name (const gchar* text)
{
	const gchar *end = NULL;
  	const gchar* p;
  	const gchar* alpha_end = NULL;
  	gint num;
  	gint name_len;
  	gchar *name_without_number;
  	gchar *result;
	
	g_return_val_if_fail (text, NULL);
	
	/* Get the end character,
	 * also doing a UTF-8 validation which is required for using g_utf8_prev_char().
	 */
	if (!g_utf8_validate (text, -1, &end))
		return NULL;
	
  	if (!end)
  		return NULL;
  		
  	--end; /* Go to before the null-termination. */
	
  	/* Look at each UTF-8 characer, starting at the end: */
  	p = end;
  	alpha_end = NULL;
  	while (p)
  	{	
  		/* Stop when we reach the first character that is not a numeric digit: */
  		const gunichar ch = g_utf8_get_char (p);
  		if (!g_unichar_isdigit (ch)) {
  			alpha_end = p;
  			break;
  		}
  		
  		p = g_utf8_find_prev_char (text, p);	
  	}
  	
  	if(!alpha_end) {
  		/* The text must consist completely of numeric digits. */
  		alpha_end = text;
  	}
  	else
  		++alpha_end;
  	
  	/* Intepret and increment the number, if any: */
  	num = atol (alpha_end);
  	++num;
  	
	/* Get the name part: */
  	name_len = alpha_end - text;
  	name_without_number = g_malloc(name_len + 1);
  	memcpy (name_without_number, text, name_len);
  	name_without_number[name_len] = 0;	\
  	
	/* Concatenate the text part and the new number: */	
  	result = g_strdup_printf("%s%d", name_without_number, num);
  	g_free (name_without_number);
  	
  	return result; 	
}

/**
 * im_account_mgr_get_unused_account_name:
 * @self: a #ImAccountMgr
 * @name: The initial account name
 *
 * get an unused account name, based on a starting string.
 *
 * Returns: (transfer full): a valid non used name
 */
gchar*
im_account_mgr_get_unused_account_name (ImAccountMgr *self,
					const gchar* starting_name,
					gboolean server_account)
{
	gchar *account_name = g_strdup (starting_name);
	
	while (im_account_mgr_account_exists (self, 
					      account_name, 
					      server_account /*  server_account */)) {
		gchar * account_name2 = util_increment_name (account_name);
		g_free (account_name);
		account_name = account_name2;
	}
	
	return account_name;
}

/**
 * im_account_mgr_get_unused_account_display name:
 * @self: a #ImAccountMgr
 * @name: The initial account display name
 *
 * get an unused account display name, based on a starting string.
 *
 * Returns: (transfer full): the unused display name.
 */
gchar*
im_account_mgr_get_unused_account_display_name (ImAccountMgr *self,
						const gchar* starting_name)
{
	gchar *account_name = g_strdup (starting_name);
	
	while (im_account_mgr_account_with_display_name_exists (self, account_name)) {
		
		gchar * account_name2 = util_increment_name (account_name);
		g_free (account_name);
		account_name = account_name2;
	}
	
	return account_name;
}

/**
 * im_account_mgr_set_leave_on_server:
 * @self: an #ImAccountMgr
 * @account_name: the account name
 * @leave_on_server: a #gboolean
 *
 * Sets if the account will leave messages on server
 * or will it store locally (used in POP).
 */
void 
im_account_mgr_set_leave_on_server (ImAccountMgr *self, 
				    const gchar *account_name, 
				    gboolean leave_on_server)
{
	im_account_mgr_set_bool (self, 
				     account_name,
				     IM_ACCOUNT_LEAVE_ON_SERVER, 
				     leave_on_server, 
				     FALSE);
}

/**
 * im_account_mgr_get_leave_on_server:
 * @self: an #ImAccountMgr
 * @account_name: the account name
 *
 * Gets if the account will leave messages on server
 * or will it store locally (used in POP).
 *
 * Returns: %TRUE if messages will be left on server, %FALSE otherwise.
 */
gboolean 
im_account_mgr_get_leave_on_server (ImAccountMgr *self, 
				    const gchar* account_name)
{
	return im_account_mgr_get_bool (self, 
					account_name,
					IM_ACCOUNT_LEAVE_ON_SERVER, 
					FALSE);
}

/**
 * im_account_mgr_get_last_updated:
 * @self: an #ImAccountMgr
 * @account_name: account name
 *
 * Obtains the timestamp of the last time the account has been
 * updated
 *
 * Returns: a timestamp.
 */
gint 
im_account_mgr_get_last_updated (ImAccountMgr *self, 
				 const gchar* account_name)
{
	return im_account_mgr_get_int (self, 
				       account_name, 
				       IM_ACCOUNT_LAST_UPDATED, 
				       TRUE);
}

/**
 * im_account_mgr_set_last_updated:
 * @self: an #ImAccountMgr
 * @account_name: account name
 * @time: new timestamp
 *
 * Sets the timestamp of the last time the account has been
 * updated
 */
void 
im_account_mgr_set_last_updated (ImAccountMgr *self, 
				     const gchar* account_name,
				     gint time)
{
	im_account_mgr_set_int (self, 
				account_name, 
				IM_ACCOUNT_LAST_UPDATED, 
				time, 
				TRUE);
	
	/* if 'account_name' is not defined, use "<null>" string */
	if (!account_name) {
		account_name = "<null>";
	}
}

/**
 * im_account_mgr_get_has_new_mails:
 * @self: an #ImAccountMgr
 * @account_name: the account name
 *
 * Obtains if the account has new messages since last time user
 * opened it.
 *
 * Returns: %TRUE if new messages have arrives
 */
gboolean
im_account_mgr_get_has_new_mails (ImAccountMgr *self, 
				  const gchar* account_name)
{
	return im_account_mgr_get_bool (self,
					account_name, 
					IM_ACCOUNT_HAS_NEW_MAILS, 
					FALSE);
}

/**
 * im_account_mgr_set_has_new_mails:
 * @self: an #ImAccountMgr
 * @account_name: the account name
 * @has_new_mails: %TRUE to indicate there are new messages in the account.
 *
 * Sets if the message has new messages user hasn't checked before.
 */
void 
im_account_mgr_set_has_new_mails (ImAccountMgr *self, 
				  const gchar* account_name,
				  gboolean has_new_mails)
{
	im_account_mgr_set_bool (self, 
				 account_name, 
				 IM_ACCOUNT_HAS_NEW_MAILS, 
				 has_new_mails, 
				 FALSE);
	
	/* TODO: notify about changes */
}

gint  
im_account_mgr_get_retrieve_limit (ImAccountMgr *self, 
				   const gchar* account_name)
{
	return im_account_mgr_get_int (self, 
				       account_name,
				       IM_ACCOUNT_LIMIT_RETRIEVE, 
				       FALSE);
}

void  
im_account_mgr_set_retrieve_limit (ImAccountMgr *self, 
				   const gchar* account_name,
				   gint limit_retrieve)
{
	im_account_mgr_set_int (self, 
				account_name,
				IM_ACCOUNT_LIMIT_RETRIEVE, 
				limit_retrieve, 
				FALSE /* not server account */);
}

gint  
im_account_mgr_get_server_account_port (ImAccountMgr *self, 
					const gchar* account_name)
{
	return im_account_mgr_get_int (self, 
				       account_name,
				       IM_ACCOUNT_PORT, 
				       TRUE);
}

void
im_account_mgr_set_server_account_port (ImAccountMgr *self, 
					const gchar *account_name,
					gint port_num)
{
	im_account_mgr_set_int (self, 
				account_name,
				IM_ACCOUNT_PORT, 
				port_num, TRUE /* server account */);
}

gchar* 
im_account_mgr_get_server_account_name (ImAccountMgr *self, 
					const gchar *account_name,
					ImAccountType account_type)
{
	return im_account_mgr_get_string (self, 
					  account_name,
					  (account_type == IM_ACCOUNT_TYPE_STORE) ?
					  IM_ACCOUNT_STORE_ACCOUNT :
					  IM_ACCOUNT_TRANSPORT_ACCOUNT, 
					  FALSE);
}

gchar*
im_account_mgr_get_server_parent_account_name (ImAccountMgr *self,
					       const char *server_account_name,
					       ImAccountType account_type)
{
	GSList *account_names, *node;
	gchar *result = NULL;
	
	account_names = im_account_mgr_account_names (self, TRUE);
	for (node = account_names; result == NULL && node != NULL; node = g_slist_next (node)) {
		char *server_name;
		
		server_name = im_account_mgr_get_server_account_name (self, (char *) node->data, account_type);
		if (g_strcmp0 (server_name, server_account_name) == 0) {
			result = g_strdup (node->data);
		}
		g_free (server_name);
	}
	
	im_account_mgr_free_account_names (account_names);
	return result;
}


static const gchar *
get_retrieve_type_name (ImAccountRetrieveType retrieve_type)
{
	switch(retrieve_type) {
	case IM_ACCOUNT_RETRIEVE_HEADERS_ONLY:
		return IM_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY;
		break;
	case IM_ACCOUNT_RETRIEVE_MESSAGES:
		return IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES;
		break;
	case IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS:
		return IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS;
		break;
	default:
		return IM_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY;
	};
}

static ImAccountRetrieveType
get_retrieve_type (const gchar *name)
{
	if (!name || name[0] == 0)
		return IM_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	if (strcmp (name, IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES) == 0) {
		return IM_ACCOUNT_RETRIEVE_MESSAGES;
	} else if (strcmp (name, IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS) == 0) {
		return IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS;
	} else {
		/* we fall back to headers only */
		return IM_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	}
}

ImAccountRetrieveType
im_account_mgr_get_retrieve_type (ImAccountMgr *self, 
				      const gchar *account_name)
{
	gchar *string;
	ImAccountRetrieveType result;
	
	string =  im_account_mgr_get_string (self, 
					     account_name,
					     IM_ACCOUNT_RETRIEVE, 
					     FALSE /* not server account */);
	result = get_retrieve_type (string);
	g_free (string);
	
	return result;
}

void 
im_account_mgr_set_retrieve_type (ImAccountMgr *self, 
				  const gchar *account_name,
				  ImAccountRetrieveType retrieve_type)
{
	im_account_mgr_set_string (self, 
				   account_name,
				   IM_ACCOUNT_RETRIEVE, 
				   get_retrieve_type_name (retrieve_type), 
				   FALSE /* not server account */);
}


void
im_account_mgr_set_user_fullname (ImAccountMgr *self, 
				  const gchar *account_name,
				  const gchar *fullname)
{
	im_account_mgr_set_string (self, 
				   account_name,
				   IM_ACCOUNT_FULLNAME, 
				   fullname, 
				   FALSE /* not server account */);
}

void
im_account_mgr_set_user_email (ImAccountMgr *self, 
			       const gchar *account_name,
			       const gchar *email)
{
	im_account_mgr_set_string (self, 
				   account_name,
				   IM_ACCOUNT_EMAIL, 
				   email, 
				   FALSE /* not server account */);
}
