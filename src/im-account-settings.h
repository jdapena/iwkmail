/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-settings.h : in memory representation of an account settings */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2007, Nokia Corporation
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


/* im-account-settings.h */

#ifndef __IM_ACCOUNT_SETTINGS_H__
#define __IM_ACCOUNT_SETTINGS_H__

#include <glib-object.h>
#include <im-server-account-settings.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_ACCOUNT_SETTINGS             (im_account_settings_get_type())
#define IM_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettings))
#define IM_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettingsClass))
#define IM_IS_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_ACCOUNT_SETTINGS))
#define IM_IS_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_ACCOUNT_SETTINGS))
#define IM_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettingsClass))

typedef struct _ImAccountSettings      ImAccountSettings;
typedef struct _ImAccountSettingsClass ImAccountSettingsClass;

struct _ImAccountSettings {
	GObject parent;
};

struct _ImAccountSettingsClass {
	GObjectClass parent_class;
};

typedef enum {
	IM_ACCOUNT_TYPE_STORE = 0,
	IM_ACCOUNT_TYPE_TRANSPORT
} ImAccountType;

typedef enum {
	IM_ACCOUNT_RETRIEVE_HEADERS_ONLY = 0,
	IM_ACCOUNT_RETRIEVE_MESSAGES,
	IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS
} ImAccountRetrieveType;


/**
 * im_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType  im_account_settings_get_type   (void) G_GNUC_CONST;

/**
 * im_account_settings_new:
 *
 * creates a new instance of #ImAccountSettings
 *
 * Returns: a #ImAccountSettings
 */
ImAccountSettings*    im_account_settings_new (void);

/**
 * im_account_settings_get_fullname:
 * @settings: a #ImAccountSettings
 *
 * get the user full name.
 *
 * Returns: a string
 */
const gchar* im_account_settings_get_fullname (ImAccountSettings *settings);

/**
 * im_account_settings_set_fullname:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @fullname as the user full name .
 */
void         im_account_settings_set_fullname (ImAccountSettings *settings,
						   const gchar *fullname);
/**
 * im_account_settings_get_email_address:
 * @settings: a #ImAccountSettings
 *
 * get the user email address.
 *
 * Returns: a string
 */
const gchar* im_account_settings_get_email_address (ImAccountSettings *settings);

/**
 * im_account_settings_set_email_address:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @email_address of the account.
 */
void         im_account_settings_set_email_address (ImAccountSettings *settings,
							const gchar *email_address);
/**
 * im_account_settings_get_retrieve_type:
 * @settings: a #ImAccountSettings
 *
 * get the account retrieve type.
 *
 * Returns: a #ImAccountRetrieveType
 */
ImAccountRetrieveType im_account_settings_get_retrieve_type (ImAccountSettings *settings);

/**
 * im_account_settings_set_retrieve_type:
 * @settings: a #ImAccountSettings
 * @retrieve_type: a #ImAccountRetrieveType.
 *
 * set @retrieve_type of the account.
 */
void         im_account_settings_set_retrieve_type (ImAccountSettings *settings,
							ImAccountRetrieveType retrieve_type);

/**
 * im_account_settings_get_retrieve_limit:
 * @settings: a #ImAccountSettings
 *
 * get the account retrieve limit. 0 is no limit.
 *
 * Returns: a #gint
 */
gint im_account_settings_get_retrieve_limit (ImAccountSettings *settings);

/**
 * im_account_settings_set_retrieve_limit:
 * @settings: a #ImAccountSettings
 * @retrieve_limit: a #gint.
 *
 * set @retrieve_limit of the account. 0 is no limit.
 */
void         im_account_settings_set_retrieve_limit (ImAccountSettings *settings,
							 gint retrieve_limit);

/**
 * im_account_settings_get_display_name:
 * @settings: a #ImAccountSettings
 *
 * get the visible name of the account.
 *
 * Returns: a string
 */
const gchar* im_account_settings_get_display_name (ImAccountSettings *settings);

/**
 * im_account_settings_set_display_name:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @display_name as the name of the account visible to the users in UI.
 */
void         im_account_settings_set_display_name (ImAccountSettings *settings,
						       const gchar *display_name);

/**
 * im_account_settings_get_account_name:
 * @settings: a #ImAccountSettings
 *
 * get the #ImAccountMgr account name for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar *im_account_settings_get_account_name (ImAccountSettings *settings);

/**
 * im_account_settings_set_account_name:
 * @settings: a #ImAccountSettings
 * @account_name: a string
 *
 * sets the account name that will be used to store the account settings. This should
 * only be called from #ImAccountMgr and #ImAccountSettings.
 */
void im_account_settings_set_account_name (ImAccountSettings *settings,
					       const gchar *account_name);

/**
 * im_account_settings_get_enabled:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the account is enabled or not.
 *
 * Returns: a #gboolean
 */
gboolean im_account_settings_get_enabled (ImAccountSettings *settings);
					      
/**
 * im_account_settings_set_enabled:
 * @settings: a #ImAccountSettings
 * @enabled: a #gboolean
 *
 * set if @settings account is enabled or not.
 */
void im_account_settings_set_enabled (ImAccountSettings *settings, gboolean enabled);


/**
 * im_account_settings_get_is_default:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the account is the default account or not.
 *
 * Returns: a #gboolean
 */
gboolean im_account_settings_get_is_default (ImAccountSettings *settings);
					      
/**
 * im_account_settings_set_is_default:
 * @settings: a #ImAccountSettings
 * @is_default: a #gboolean
 *
 * set if @settings account is the default account or not.
 */
void im_account_settings_set_is_default (ImAccountSettings *settings, gboolean is_default);

/**
 * im_account_settings_get_store_settings:
 * @settings: a #ImAccountSettings
 *
 * obtains a ref'ed instance of the store account server settings
 *
 * Returns: a ref'd #ImServerAccountSettings. You should unreference it on finishing usage.
 */
ImServerAccountSettings *im_account_settings_get_store_settings (ImAccountSettings *settings);

/**
 * im_account_settings_set_store_settings:
 * @settings: a #ImAccountSettings
 *
 * sets @store_settings as the settings of the store account of @settings account.
 * @settings will keep an internal reference to it.
 */
void im_account_settings_set_store_settings (ImAccountSettings *settings, 
						 ImServerAccountSettings *store_settings);

/**
 * im_account_settings_get_transport_settings:
 * @settings: a #ImAccountSettings
 *
 * obtains a ref'ed instance of the transport account server settings
 *
 * Returns: a ref'd #ImServerAccountSettings. You should unreference it on finishing usage.
 */
ImServerAccountSettings *im_account_settings_get_transport_settings (ImAccountSettings *settings);

/**
 * im_account_settings_set_transport_settings:
 * @settings: a #ImAccountSettings
 *
 * sets @transport_settings as the settings of the transport account of @settings account.
 * @settings will keep an internal reference to it.
 */
void im_account_settings_set_transport_settings (ImAccountSettings *settings, 
						     ImServerAccountSettings *transport_settings);

/**
 * im_account_settings_get_use_signature:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the mails from this account use signature or not.
 *
 * Returns: a #gboolean
 */
gboolean im_account_settings_get_use_signature (ImAccountSettings *settings);
					      
/**
 * im_account_settings_set_use_signature:
 * @settings: a #ImAccountSettings
 * @use_signature: a #gboolean
 *
 * set if @settings mails use signature or not
 */
void im_account_settings_set_use_signature (ImAccountSettings *settings, gboolean use_signature);

/**
 * im_account_settings_get_signature:
 * @settings: a #ImAccountSettings
 *
 * get the signature.
 *
 * Returns: a string
 */
const gchar* im_account_settings_get_signature (ImAccountSettings *settings);

/**
 * im_account_settings_set_signature:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @signature for the account .
 */
void         im_account_settings_set_signature (ImAccountSettings *settings,
						   const gchar *signature);
/**
 * im_account_settings_get_leave_messages_on_server:
 * @settings: a #ImAccountSettings
 *
 * obtains whether messages should be left on server or not
 *
 * Returns: a #gboolean
 */
gboolean im_account_settings_get_leave_messages_on_server (ImAccountSettings *settings);
					      
/**
 * im_account_settings_set_leave_messages_on_server:
 * @settings: a #ImAccountSettings
 * @leave_messages_on_server: a #gboolean
 *
 * set if we leave the messages on server or not.
 */
void im_account_settings_set_leave_messages_on_server (ImAccountSettings *settings, 
							   gboolean leave_messages_on_server);


/**
 * im_account_settings_get_use_connection_specific_smtp:
 * @settings: a #ImAccountSettings
 *
 * obtains if we should try the connection specific smtp servers
 *
 * Returns: a #gboolean
 */
gboolean im_account_settings_get_use_connection_specific_smtp (ImAccountSettings *settings);
					      
/**
 * im_account_settings_set_use_connection_specific_smtp:
 * @settings: a #ImAccountSettings
 * @use_connection_specific_smtp: a #gboolean
 *
 * if set, mails sent from this account first try the connection specific smtp servers
 * before the transport account.
 */
void im_account_settings_set_use_connection_specific_smtp (ImAccountSettings *settings, 
							       gboolean use_connection_specific_smtp);

G_END_DECLS

#endif /* __IM_ACCOUNT_SETTINGS_H__ */
