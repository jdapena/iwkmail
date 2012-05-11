/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-mgr-priv.h :  Private methods for ImAccountMgr */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Dirk-Jan C. Binnema
 *  Johannes Schmid
 *  Murray Cumming
 *  Philip Van Hoof
 *
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

#ifndef __IM_ACCOUNT_MGR_PRIV_H__
#define __IM_ACCOUNT_MGR_PRIV_H__

#include <glib.h>
#include <im-conf.h>

/* configuration key definitions for im */
#define IM_ACCOUNT_SUBNAMESPACE      "/accounts"
#define IM_ACCOUNT_NAMESPACE         (im_defs_namespace (IM_ACCOUNT_SUBNAMESPACE))
#define IM_CONF_DEFAULT_ACCOUNT      (im_defs_namespace ("/default_account"))

#define IM_SERVER_ACCOUNT_SUBNAMESPACE "/server_accounts"
#define IM_SERVER_ACCOUNT_NAMESPACE  (im_defs_namespace (IM_SERVER_ACCOUNT_SUBNAMESPACE))

/* per-account data */
#define IM_ACCOUNT_DISPLAY_NAME      "display_name"      /* string */
#define IM_ACCOUNT_STORE_ACCOUNT     "store_account"     /* string */
#define IM_ACCOUNT_TRANSPORT_ACCOUNT "transport_account" /* string */
#define IM_ACCOUNT_FULLNAME		 "fullname"          /* string */
#define IM_ACCOUNT_EMAIL             "email"             /* string */

/* This is a list of strings, with each strings, 
 * alernating between a connection name, followed by a corresponding server account name.
 * That's not pretty, but it's nicer than dealing with escaping of a = separator if 
 * putting them both in one string. */
#define IM_CONF_CONNECTION_SPECIFIC_SMTP_LIST \
	(im_defs_namespace ("/specific_smtp")) /* one list used for all accounts. */
#define IM_ACCOUNT_USE_CONNECTION_SPECIFIC_SMTP  "use_specific_smtp" /* boolean */

/* server account keys */
#define IM_ACCOUNT_HOSTNAME          "hostname"          /* string */
#define IM_ACCOUNT_USERNAME          "username"          /* string */
#define IM_ACCOUNT_USERNAME_HAS_SUCCEEDED          "username_succeeded"          /* string */
#define IM_ACCOUNT_USE_SIGNATURE         "use_signature"         /* boolean */
#define IM_ACCOUNT_SIGNATURE         "signature"         /* string */

/* Only used for mbox and maildir accounts: */
#define IM_ACCOUNT_URI		 "uri"	             /* string */

#define IM_ACCOUNT_PROTO             "proto"             /* string */
#define IM_ACCOUNT_ENABLED		 "enabled"	     /* boolean */
#define IM_ACCOUNT_TYPE		 "type"	             /* string */
#define IM_ACCOUNT_LAST_UPDATED      "last_updated"      /* int */
#define IM_ACCOUNT_HAS_NEW_MAILS     "has_new_mails"     /* boolean */

#define IM_ACCOUNT_LEAVE_ON_SERVER   "leave_on_server"   /* boolean */
#define IM_ACCOUNT_PREFERRED_CNX     "preferred_cnx"     /* string */
#define IM_ACCOUNT_PORT		         "port"	             /* int */

#define IM_ACCOUNT_AUTH_MECH	 "auth_mech"	     /* string */
#define IM_ACCOUNT_AUTH_MECH_VALUE_NONE "none"
#define IM_ACCOUNT_AUTH_MECH_VALUE_PASSWORD "password"
#define IM_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5 "cram-md5"

#define IM_ACCOUNT_RETRIEVE	 "retrieve"	     /* string */
#define IM_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY "headers-only"
#define IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES "messages"
#define IM_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS "messages-and-attachments"

#define IM_ACCOUNT_LIMIT_RETRIEVE	 "limit-retrieve"	     /* int */

#define IM_ACCOUNT_SECURITY "security"
#define IM_ACCOUNT_SECURITY_VALUE_NONE "none"
#define IM_ACCOUNT_SECURITY_VALUE_NORMAL "normal" /* Meaning "Normal (TLS)", as in our UI spec. */ 
#define IM_ACCOUNT_SECURITY_VALUE_SSL "ssl"



/*
 * private functions, only for use in im-account-mgr and
 * im-account-mgr-helpers
 */

G_BEGIN_DECLS

gchar* _im_account_mgr_account_from_key (const gchar *key, gboolean *is_account_key,
					     gboolean *is_server_account);
gchar * _im_account_mgr_get_account_keyname (const gchar *account_name, const gchar * name,
						 gboolean server_account);

/* below is especially very _private_ stuff */
typedef struct _ImAccountMgrPrivate ImAccountMgrPrivate;
struct _ImAccountMgrPrivate {
	ImConf        *im_conf;
	
	/* We store these as they change, and send notifications every X seconds: */
	GSList* busy_accounts;

	guint timeout;
	
	GHashTable *notification_id_accounts;
	GHashTable *server_account_key_hash;
	GHashTable *account_key_hash;
	
	/* cache whether we have accounts; if this is TRUE, we have accounts, if
	 * it's FALSE we _don't know_ if we have account and need to check
	 */
	gboolean has_accounts;
	gboolean has_enabled_accounts;
};
#define IM_ACCOUNT_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
	         				    IM_TYPE_ACCOUNT_MGR, \
                                                ImAccountMgrPrivate))
	
/**
 * im_account_mgr_set_bool:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config bool for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	im_account_mgr_set_bool       (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gboolean val,
						   gboolean server_account);

/**
 * im_account_mgr_get_bool:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config boolean from an account
 *
 * Returns: an boolean with the value for the key, or FALSE in case of
 * error (but of course FALSE does not necessarily imply an error)
 */
gboolean	im_account_mgr_get_bool       (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);


/**
 * im_account_mgr_get_list:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to get
 * @list_type: the type of the members of the list
 * @server_account: if TRUE, this is a server account
 * 
 * get a config list of values of type @list_type of an account
 *
 * Returns: a newly allocated list of elements
 */
GSList*	        im_account_mgr_get_list       (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   ImConfValueType list_type,
						   gboolean server_account);

/**
 * im_account_mgr_set_list:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the list with the values to set
 * @list_type: the type of the members of the list
 * @server_account: if TRUE, this is a server account
 *
 * * set a config list of values of type @list_type of an account
 * 
 * returns TRUE if this succeeded, FALSE otherwise 
 */
gboolean	        im_account_mgr_set_list       (ImAccountMgr *self,
							   const gchar *name,
							   const gchar *key,
							   GSList *val,
							   ImConfValueType list_type,
							   gboolean server_account);

/**
 * im_account_mgr_get_int:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config int from an account
 *
 * Returns: an integer with the value for the key, or -1 in case of
 * error (but of course -1 does not necessarily imply an error)
 */
gint	        im_account_mgr_get_int        (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);



/**
 * im_account_mgr_set_int:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config int for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	im_account_mgr_set_int        (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gint val,
						   gboolean server_account);

/**
 * im_account_mgr_get_string:
 * @self: self a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config string from an account
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. 
 */
gchar*	        im_account_mgr_get_string     (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);


/**
 * im_account_mgr_set_string:
 * @self: a ImAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config string for an account.
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	im_account_mgr_set_string     (ImAccountMgr *self,
						   const gchar *name,
						   const gchar *key, const gchar* val,
						   gboolean server_account);

G_END_DECLS
#endif /* __IM_ACCOUNT_MGR_PRIV_H__ */
