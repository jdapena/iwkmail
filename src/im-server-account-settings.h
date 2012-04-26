/* Based on ModestServerAccountSettings */

/* Copyright (c) 2007, Nokia Corporation
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


/* im-server-account-settings.h */

#ifndef __IM_SERVER_ACCOUNT_SETTINGS_H__
#define __IM_SERVER_ACCOUNT_SETTINGS_H__

#include <glib-object.h>
#include <im-protocol.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_SERVER_ACCOUNT_SETTINGS             (im_server_account_settings_get_type())
#define IM_SERVER_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettings))
#define IM_SERVER_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettingsClass))
#define IM_IS_SERVER_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS))
#define IM_IS_SERVER_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_SERVER_ACCOUNT_SETTINGS))
#define IM_SERVER_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettingsClass))

typedef struct _ImServerAccountSettings      ImServerAccountSettings;
typedef struct _ImServerAccountSettingsClass ImServerAccountSettingsClass;

struct _ImServerAccountSettings {
	GObject parent;
};

struct _ImServerAccountSettingsClass {
	GObjectClass parent_class;
};


/**
 * im_server_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType  im_server_account_settings_get_type   (void) G_GNUC_CONST;

/**
 * im_server_account_settings_new:
 *
 * creates a new instance of #ImServerAccountSettings
 *
 * Returns: a #ImServerAccountSettings
 */
ImServerAccountSettings*    im_server_account_settings_new (void);

/**
 * im_server_account_settings_get_hostname:
 * @settings: a #ImServerAccountSettings
 *
 * get the server hostname.
 *
 * Returns: a string
 */
const gchar* im_server_account_settings_get_hostname (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_hostname:
 * @settings: a #ImServerAccountSettings
 * @hostname: a string.
 *
 * set @hostname as the server hostname.
 */
void         im_server_account_settings_set_hostname (ImServerAccountSettings *settings,
								      const gchar *hostname);

/**
 * im_server_account_settings_get_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the server protocol.
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType im_server_account_settings_get_protocol (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_protocol:
 * @settings: a #ImServerAccountSettings
 * @protocol: a #ImProtocolType
 *
 * set @server_type.
 */
void                          im_server_account_settings_set_protocol (ImServerAccountSettings *settings,
									   ImProtocolType protocol_type);


/**
 * im_server_account_settings_get_uri:
 * @settings: a #ImServerAccountSettings
 *
 * get the uri, if any. If this is set, then all the other fields are invalid. It's only valid if protocol is %NULL.
 *
 * Returns: a string
 */
const gchar *im_server_account_settings_get_uri (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_uri:
 * @settings: a #ImServerAccountSettings
 * @uri: a string
 *
 * set @uri. When you set an @uri, then the protocol is set to %IM_PROTOCOL_REGISTRY_TYPE_INVALID. This is used for setting maildir or mbox
 * accounts.
 */
void   im_server_account_settings_set_uri (ImServerAccountSettings *settings,
					       const gchar *uri);

/**
 * im_server_account_settings_get_port:
 * @settings: a #ImServerAccountSettings
 *
 * get the server port.
 *
 * Returns: a #guint
 */
guint  im_server_account_settings_get_port (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_port:
 * @settings: a #ImServerAccountSettings
 * @port: a #guint.
 *
 * set @port.
 */
void   im_server_account_settings_set_port (ImServerAccountSettings *settings,
						guint port);

/**
 * im_server_account_settings_get_username:
 * @settings: a #ImServerAccountSettings
 *
 * get the username.
 *
 * Returns: a string
 */
const gchar *im_server_account_settings_get_username (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_username:
 * @settings: a #ImServerAccountSettings
 * @username: a string
 *
 * set @username.
 */
void   im_server_account_settings_set_username (ImServerAccountSettings *settings,
						    const gchar *username);

/**
 * im_server_account_settings_get_password:
 * @settings: a #ImServerAccountSettings
 *
 * get the password.
 *
 * Returns: a string
 */
const gchar *im_server_account_settings_get_password (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_password:
 * @settings: a #ImServerAccountSettings
 * @password: a string
 *
 * set @password.
 */
void   im_server_account_settings_set_password (ImServerAccountSettings *settings,
						    const gchar *password);


/**
 * im_server_account_settings_get_security_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the secure connection type, if any.
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType im_server_account_settings_get_security_protocol (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_security_protocol:
 * @settings: a #ImServerAccountSettings
 * @security: a #ImProtocolType
 *
 * set the current security connection protocol to @security.
 */
void   im_server_account_settings_set_security_protocol (ImServerAccountSettings *settings,
							     ImProtocolType security_protocol);


/**
 * im_server_account_settings_get_auth_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the authentication protocol
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType im_server_account_settings_get_auth_protocol (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_auth_protocol:
 * @settings: a #ImServerAccountSettings
 * @auth_protocol: a #ImProtocolType
 *
 * set the current authentication protocol to @auth_protocol.
 */
void   im_server_account_settings_set_auth_protocol (ImServerAccountSettings *settings,
							 ImProtocolType auth_protocol);

/**
 * im_server_account_settings_get_account_name:
 * @settings: a #ImServerAccountSettings
 *
 * get the #ImAccountMgr account name for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar *im_server_account_settings_get_account_name (ImServerAccountSettings *settings);

/**
 * im_server_account_settings_set_account_name:
 * @settings: a #ImServerAccountSettings
 * @account_name: a string
 *
 * sets the account name that will be used to store the account settings. This should
 * only be called from #ImAccountMgr and #ImAccountSettings.
 */
void im_server_account_settings_set_account_name (ImServerAccountSettings *settings,
						      const gchar *account_name);


G_END_DECLS

#endif /* __IM_SERVER_ACCOUNT_SETTINGS_H__ */
