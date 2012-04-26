/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-mgr.c : account settings persistent storage */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Felipe Erias Morandeira <femorandeira@igalia.com>
 *  Dirk-Jan C. Binnema
 *  Philip Van Hoof
 *  Murray Cumming
 *  Arne Zellentin
 *  Johannes Schmid
 *  Nils Faerber
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


#ifndef __IM_ACCOUNT_MGR_H__
#define __IM_ACCOUNT_MGR_H__

#include <im-conf.h>
#include <im-account-settings.h>
#include <im-protocol-registry.h>

#include <glib-object.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_ACCOUNT_MGR             (im_account_mgr_get_type())
#define IM_ACCOUNT_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_ACCOUNT_MGR,ImAccountMgr))
#define IM_ACCOUNT_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_ACCOUNT_MGR,ImAccountMgrClass))
#define IM_IS_ACCOUNT_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_ACCOUNT_MGR))
#define IM_IS_ACCOUNT_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_ACCOUNT_MGR))
#define IM_ACCOUNT_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_ACCOUNT_MGR,ImAccountMgrClass))

typedef struct _ImAccountMgr      ImAccountMgr;
typedef struct _ImAccountMgrClass ImAccountMgrClass;

struct _ImAccountMgr {
	 GObject parent;
};

struct _ImAccountMgrClass {
	GObjectClass parent_class;

	void         (* account_inserted)               (ImAccountMgr *obj, 
							 const gchar* account,
							 gpointer user_data);

	void         (* account_removed)                (ImAccountMgr *obj, 
							 const gchar* account,
							 gpointer user_data);
	
	void         (* account_changed)                (ImAccountMgr *obj, 
							 const gchar* account,
							 gpointer user_data);

	void         (* account_busy_changed)           (ImAccountMgr *obj, 
							 const gchar* account,
							 gboolean busy,
							 gpointer user_data);	

	void         (* default_account_changed)        (ImAccountMgr *obj, 
							 gpointer user_data);

	void         (* display_name_changed)           (ImAccountMgr *obj, 
							 const gchar *account,
							 gpointer user_data);
	
	void         (* account_updated)                (ImAccountMgr *obj, 
							 const gchar *account,
							 gpointer user_data);
};

/**
 * im_account_mgr_get_type:
 * 
 * get the GType for ImAccountMgr
 *  
 * Returns: the GType
 */
GType           im_account_mgr_get_type       (void) G_GNUC_CONST;


/**
 * im_account_mgr_get_instance:
 *  
 * Returns: the #ImAccountMgr singleton, or %NULL in case of error
 */
ImAccountMgr*        im_account_mgr_get_instance   (void);


/**
 * im_account_mgr_add_account_from_settings:
 * @self: a #ImAccountMgr instance
 * @self: a #ImSettings
 * 
 * Create a new account from a @settings instance.
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 */
gboolean        im_account_mgr_add_account_from_settings    (ImAccountMgr *self,
							     ImAccountSettings *settings);

/**
 * im_account_mgr_add_account:
 * @self: a ImAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @store_name: the store account (ie. POP/IMAP)
 * @transport_name: the transport account (ie. sendmail/SMTP)
 * @enabled: Whether the account should be enabled initially.
 * 
 * Create a new account. The account with @name should not already exist. The @name will 
 * be used as the initial display name of the new account.
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 */
gboolean        im_account_mgr_add_account    (ImAccountMgr *self,
					       const gchar *name,
					       const gchar *display_name,
					       const gchar *user_fullname,
					       const gchar *user_email,
					       ImAccountRetrieveType retrieve_type,
					       const gchar* store_name,
					       const gchar* transport_name,
					       gboolean enabled);

/**
 * im_account_mgr_add_server_account:
 * @self: a ImAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @hostname: the hostname
 * @portnumber: the portnumber, or 0 for default
 * @username: the username
 * @password: the password
 * @proto:    the protocol (imap, smtp, ...) used for this account
 * @security: the security options, (SSL, TLS ...) used to access the server
 * @auth: the authentication method (password, none ...) used to access the server
 * 
 * add a server account to the configuration.
 * the server account with @name should not already exist
 * 
 * Returns: TRUE if succeeded, FALSE otherwise,
 */
gboolean im_account_mgr_add_server_account    (ImAccountMgr *self,
					       const gchar *name,
					       const gchar *hostname,
					       const guint portnumber,
					       const gchar *username,
					       const gchar *password,
					       ImProtocolType proto,
					       ImProtocolType security,
					       ImProtocolType auth);


/**
 * im_account_mgr_add_server_account_uri:
 * @self: a ImAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @proto:    the protocol (imap, smtp, ...) used for this account
 * @uri: the URI
 * 
 * add a server account to the configuration, based on the account-URI
 * 
 * Returns: TRUE if succeeded, FALSE otherwise,
 */
gboolean im_account_mgr_add_server_account_uri    (ImAccountMgr *self,
						   const gchar *name,
						   ImProtocolType proto,
						   const gchar* uri);

/**
 * im_account_mgr_remove_account:
 * @self: a ImAccountMgr instance
 * @name: the name of the account to remove
 * @err: a #GError ptr, or NULL to ignore.
 * 
 * remove an existing account. the account with @name should already exist; note
 * that when deleting an account, also the corresponding server accounts will
 * be deleted
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean        im_account_mgr_remove_account         (ImAccountMgr *self,
						       const gchar* name);

/**
 * im_account_mgr_remove_account:
 * @self: a ImAccountMgr instance
 * @name: the name of the server account to remove
 * 
 * remove an existing server account. This is only for internal use.
 *
 * Returns: TRUE if the operation succeeded, FALSE otherwise,
 */
gboolean        im_account_mgr_remove_server_account         (ImAccountMgr *self,
							      const gchar* name);

/**
 * im_account_mgr_account_names:
 * @self: a ImAccountMgr instance
 * @only_enabled: Whether only enabled accounts should be returned.
 * 
 * list all account names
 *
 * Returns: a newly allocated list of account names, or NULL in case of error or
 * if there are no accounts. The caller must free the returned GSList.
 *
 */
GSList*	        im_account_mgr_account_names    (ImAccountMgr *self,
						 gboolean only_enabled);

/**
 * im_account_mgr_free_account_names:
 * @account_name: a gslist of account names
 * 
 * list all account names
 *
 * free the list of account names
 */
void	        im_account_mgr_free_account_names    (GSList *account_names);
							  

/**
 * im_account_mgr_account_exists:
 * @self: a ImAccountMgr instance
 * @name: the account name to check
 * @server_account: if TRUE, this is a server account
 * 
 * check whether account @name exists. Note that this does not check the display name.
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 */
gboolean	im_account_mgr_account_exists	  (ImAccountMgr *self,
						   const gchar *name,
						   gboolean server_account);

/**
 * im_account_mgr_account_exists:
 * @self: a ImAccountMgr instance
 * @name: the account name to check
 * 
 * check whether a non-server account with the @display_name exists.
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 */
gboolean	im_account_mgr_account_with_display_name_exists (ImAccountMgr *self,
								 const gchar *display_name);

/**
 * im_account_mgr_check_already_configured_account:
 * @self: a #ImAccountMgr
 * @settings: a #ImAccountSettings *settings
 *
 * Checks if there's already an active store account with the same settings
 *
 * Returns: %TRUE if account setup exists
 */
gboolean        im_account_mgr_check_already_configured_account (ImAccountMgr * self,
								 ImAccountSettings *settings);

/**
 * im_account_mgr_unset:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to unset
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * unsets the config value of an account and all their children keys
 *
 * Returns: TRUE if unsetting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean        im_account_mgr_unset           (ImAccountMgr *self,
						const gchar *name,
						const gchar *key,
						gboolean server_account);

/**
 * im_account_mgr_has_accounts:
 * @self: a ImAccountMgr instance
 * @enabled: TRUE to search for enabled accounts only
 * 
 * Checks if any accounts exist
 *
 * Returns: TRUE if accounts exist, FALSE otherwise
 */

gboolean im_account_mgr_has_accounts (ImAccountMgr* self,
				      gboolean enabled);

/**
 * im_account_mgr_set_account_busy
 * @self: a ImAccountMgr instance
 * @account_name: name of the account
 * @busy: whether to set busy or not busy
 * 
 * Changes the busy flag of an account
 *
 */

void im_account_mgr_set_account_busy(ImAccountMgr* self,
				     const gchar* account_name, 
				     gboolean busy);

/**
 * im_account_mgr_account_is_busy
 * @self: a ImAccountMgr instance
 * @account_name: name of the account
 * 
 * Returns: If the account is currently busy or not
 *
 */
gboolean im_account_mgr_account_is_busy (ImAccountMgr* self, 
					 const gchar* account_name);


void im_account_mgr_notify_account_update (ImAccountMgr* self, 
					   const gchar *server_account_name);

/**
 * im_account_mgr_set_default_account:
 * @self: a ImAccountMgr instance
 * @account: the name of an existing account
 * 
 * set the default account name (which must be valid account)
 *
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean im_account_mgr_set_default_account  (ImAccountMgr *self,
					      const gchar* account);

/**
 * im_account_mgr_get_default_account:
 * @self: a ImAccountMgr instance
 * 
 * get the default account name, or NULL if none is found
 *
 * Returns: the default account name (as newly allocated string, which
 * must be g_free'd), or NULL
 */
gchar* im_account_mgr_get_default_account  (ImAccountMgr *self);

/**
 * im_account_mgr_get_display_name:
 * @self: a ImAccountMgr instance
 * @name: the account name to check
 *
 * Return the human-readable account title for this account, or NULL.
 */
gchar* im_account_mgr_get_display_name (ImAccountMgr *self, 
					const gchar* name);

void  im_account_mgr_set_display_name (ImAccountMgr *self, 
				       const gchar *account_name,
				       const gchar *display_name);

gboolean im_account_mgr_singleton_protocol_exists (ImAccountMgr *mgr,
						   ImProtocolType protocol_type);

gchar * im_account_mgr_get_string (ImAccountMgr *self,
				   const gchar *name,
				   const gchar *key,
				   gboolean server_account);
GSList * im_account_mgr_get_list (ImAccountMgr *self,
				  const gchar *name,
				  const gchar *key,
				  ImConfValueType list_type,
				  gboolean server_account);
gboolean im_account_mgr_set_list (ImAccountMgr *self,
				  const gchar *name,
				  const gchar *key,
				  GSList *val,
				  ImConfValueType list_type,
				  gboolean server_account);
G_END_DECLS

#endif /* __IM_ACCOUNT_MGR_H__ */
