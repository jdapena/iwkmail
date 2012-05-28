/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-server-account-settings.c : settings for a CamelService */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Vivek Sekar <ext-vivek.1.sekar@nokia.com>
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

#include <im-server-account-settings.h>

#include <im-protocol-registry.h>

#include <strings.h>
#include <string.h>

enum {
	PROP_0,
	PROP_HOSTNAME,
	PROP_PORT,
	PROP_PROTOCOL,
	PROP_PROTOCOL_NAME,
	PROP_SECURITY_PROTOCOL,
	PROP_SECURITY_PROTOCOL_NAME,
	PROP_AUTH_PROTOCOL,
	PROP_AUTH_PROTOCOL_NAME,
	PROP_USERNAME,
	PROP_PASSWORD,
	PROP_ACCOUNT_NAME,
	PROP_URI
};

/* 'private'/'protected' functions */
static void   im_server_account_settings_class_init     (ImServerAccountSettingsClass *klass);
static void   im_server_account_settings_finalize       (GObject *obj);
static void   im_server_account_settings_get_property   (GObject *obj,
							 guint property_id,
							 GValue *value,
							 GParamSpec *pspec);
static void   im_server_account_settings_set_property   (GObject *obj,
							 guint property_id,
							 const GValue *value,
							 GParamSpec *pspec);
static void   im_server_account_settings_instance_init  (ImServerAccountSettings *obj);

typedef struct _ImServerAccountSettingsPrivate ImServerAccountSettingsPrivate;
struct _ImServerAccountSettingsPrivate {
	gchar *hostname;
	guint port;
	ImProtocolType protocol;
	gchar *username;
	gchar *password;
	ImProtocolType security_protocol;
	ImProtocolType auth_protocol;
	gchar *account_name;
	gchar *uri;
};

#define IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
							   IM_TYPE_SERVER_ACCOUNT_SETTINGS, \
							   ImServerAccountSettingsPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

/**
 * im_server_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType
im_server_account_settings_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImServerAccountSettingsClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_server_account_settings_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImServerAccountSettings),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_server_account_settings_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImServerAccountSettings",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_server_account_settings_class_init (ImServerAccountSettingsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->get_property = im_server_account_settings_get_property;
	gobject_class->set_property = im_server_account_settings_set_property;
	gobject_class->finalize = im_server_account_settings_finalize;

	g_object_class_install_property (
		gobject_class, PROP_HOSTNAME,
		g_param_spec_string ("hostname",
				     _("Host name"),
				     _("Name of the host to connect"),
				     NULL /* default value */,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PORT,
		g_param_spec_uint ("port",
				   _("Port"),
				   _("Port used for connection to hostname"),
				   0, /* minimum value */
				   65535, /* maximum value */
				   0, /* default value */
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PROTOCOL,
		g_param_spec_uint ("protocol",
				   _("Protocol"),
				   _("Protocol for this service"),
				   0, /* minimum value */
				   IM_PROTOCOL_TYPE_INVALID, /* maximum value */
				   IM_PROTOCOL_TYPE_INVALID, /* default value */
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PROTOCOL_NAME,
		g_param_spec_string ("protocol-name",
				   _("Protocol name"),
				   _("Protocol name for this service"),
				   NULL,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PROTOCOL,
		g_param_spec_uint ("security-protocol",
				   _("Security Protocol"),
				   _("Security transport protocol used for connecting"),
				   0, /* minimum value */
				   IM_PROTOCOL_TYPE_INVALID, /* maximum value */
				   IM_PROTOCOL_TYPE_INVALID, /* default value */
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_SECURITY_PROTOCOL_NAME,
		g_param_spec_string ("security-protocol-name",
				   _("Security protocol name"),
				   _("Security transport protocol name"),
				   NULL,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PROTOCOL,
		g_param_spec_uint ("auth-protocol",
				   _("Auth Protocol"),
				   _("Authentication protocol used for connecting"),
				   0, /* minimum value */
				   IM_PROTOCOL_TYPE_INVALID, /* maximum value */
				   IM_PROTOCOL_TYPE_INVALID, /* default value */
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_AUTH_PROTOCOL_NAME,
		g_param_spec_string ("auth-protocol-name",
				   _("Auth protocol name"),
				   _("Authentication protocol name"),
				   NULL,
				   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_USERNAME,
		g_param_spec_string ("username",
				     _("User name"),
				     _("Username used on service authentication"),
				     NULL /* default value */,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_PASSWORD,
		g_param_spec_string ("password",
				     _("Password"),
				     _("Password for service authentication"),
				     NULL /* default value */,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_ACCOUNT_NAME,
		g_param_spec_string ("account-name",
				     _("Account name"),
				     _("Name of the server account in persistent storage"),
				     NULL /* default value */,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		gobject_class, PROP_URI,
		g_param_spec_string ("uri",
				     _("URI"),
				     _("URI of the service"),
				     NULL /* default value */,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_type_class_add_private (gobject_class,
				  sizeof(ImServerAccountSettingsPrivate));
}

static void
im_server_account_settings_instance_init (ImServerAccountSettings *obj)
{
	ImServerAccountSettingsPrivate *priv;

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (obj);

	priv->hostname = NULL;
	priv->protocol = IM_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->port = 0;
	priv->username = NULL;
	priv->password = NULL;
	priv->security_protocol = IM_PROTOCOLS_CONNECTION_NONE;
	priv->auth_protocol = IM_PROTOCOLS_AUTH_NONE;
	priv->account_name = NULL;
	priv->uri = NULL;
}

static void   
im_server_account_settings_finalize   (GObject *obj)
{
	ImServerAccountSettings *settings = IM_SERVER_ACCOUNT_SETTINGS (obj);
	ImServerAccountSettingsPrivate *priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->hostname);
	priv->hostname = NULL;
	g_free (priv->username);
	priv->username = NULL;

	if (priv->password) {
		bzero (priv->password, strlen (priv->password));
		g_free (priv->password);
	}
	priv->password = NULL;

	priv->protocol = IM_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->port = 0;
	priv->security_protocol = IM_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->auth_protocol = IM_PROTOCOL_REGISTRY_TYPE_INVALID;
	g_free (priv->account_name);
	priv->account_name = NULL;
	g_free (priv->uri);
	priv->uri = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
im_server_account_settings_get_property (GObject *obj,
					 guint property_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	ImServerAccountSettings *self = IM_SERVER_ACCOUNT_SETTINGS (obj);

	switch (property_id) {
	case PROP_HOSTNAME:
		g_value_set_string (value,
				    im_server_account_settings_get_hostname (self));
		break;
	case PROP_PORT:
		g_value_set_uint (value,
				  im_server_account_settings_get_port (self));
		break;
	case PROP_PROTOCOL:
		g_value_set_uint (value,
				  im_server_account_settings_get_protocol (self));
		break;
	case PROP_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_type
				(im_protocol_registry_get_instance (),
				 im_server_account_settings_get_protocol (self));
			g_value_set_string (value, 
					    protocol?im_protocol_get_name (protocol):NULL);
		}
		break;
	case PROP_SECURITY_PROTOCOL:
		g_value_set_uint (value,
				  im_server_account_settings_get_security_protocol (self));
		break;
	case PROP_SECURITY_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_type
				(im_protocol_registry_get_instance (),
				 im_server_account_settings_get_security_protocol (self));
			g_value_set_string (value, 
					    protocol?im_protocol_get_name (protocol):NULL);
		}
		break;
	case PROP_AUTH_PROTOCOL:
		g_value_set_uint (value,
				  im_server_account_settings_get_auth_protocol (self));
		break;
	case PROP_AUTH_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_type
				(im_protocol_registry_get_instance (),
				 im_server_account_settings_get_auth_protocol (self));
			g_value_set_string (value, 
					    protocol?im_protocol_get_name (protocol):NULL);
		}
		break;
	case PROP_USERNAME:
		g_value_set_string (value,
				    im_server_account_settings_get_username (self));
		break;
	case PROP_PASSWORD:
		g_value_set_string (value,
				    im_server_account_settings_get_password (self));
		break;
	case PROP_ACCOUNT_NAME:
		g_value_set_string (value,
				    im_server_account_settings_get_account_name (self));
		break;
	case PROP_URI:
		g_value_set_string (value,
				    im_server_account_settings_get_uri (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

static void
im_server_account_settings_set_property (GObject *obj,
					 guint property_id,
					 const GValue *value,
					 GParamSpec *pspec)
{
	ImServerAccountSettings *self = IM_SERVER_ACCOUNT_SETTINGS (obj);

	switch (property_id) {
	case PROP_HOSTNAME:
		im_server_account_settings_set_hostname (self,
							 g_value_get_string (value));
		break;
	case PROP_PORT:
		im_server_account_settings_set_port (self, 
						     g_value_get_uint (value));
		break;
	case PROP_PROTOCOL:
		im_server_account_settings_set_protocol (self,
							 g_value_get_uint (value));
		break;
	case PROP_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_name
				(im_protocol_registry_get_instance (),
				 IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				 g_value_get_string (value));
			im_server_account_settings_set_protocol
				(self,
				 protocol?im_protocol_get_type_id (protocol):IM_PROTOCOL_TYPE_INVALID);
		}
		break;
	case PROP_SECURITY_PROTOCOL:
		im_server_account_settings_set_security_protocol (self,
								  g_value_get_uint (value));
		break;
	case PROP_SECURITY_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_name
				(im_protocol_registry_get_instance (),
				 IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				 g_value_get_string (value));
			im_server_account_settings_set_security_protocol
				(self,
				 protocol?im_protocol_get_type_id (protocol):IM_PROTOCOL_TYPE_INVALID);
		}
		break;
	case PROP_AUTH_PROTOCOL:
		im_server_account_settings_set_auth_protocol (self,
							      g_value_get_uint (value));
		break;
	case PROP_AUTH_PROTOCOL_NAME:
		{
			ImProtocol *protocol;
			protocol = im_protocol_registry_get_protocol_by_name
				(im_protocol_registry_get_instance (),
				 IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				 g_value_get_string (value));
			im_server_account_settings_set_auth_protocol
				(self,
				 protocol?im_protocol_get_type_id (protocol):IM_PROTOCOL_TYPE_INVALID);
		}
		break;
	case PROP_USERNAME:
		im_server_account_settings_set_username (self, 
							 g_value_get_string (value));
		break;
	case PROP_PASSWORD:
		im_server_account_settings_set_password (self,
							 g_value_get_string (value));
		break;
	case PROP_ACCOUNT_NAME:
		im_server_account_settings_set_account_name (self,
							     g_value_get_string (value));
		break;
	case PROP_URI:
		im_server_account_settings_set_uri (self,
						    g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

/**
 * im_server_account_settings_new:
 *
 * creates a new instance of #ImServerAccountSettings
 *
 * Returns: a #ImServerAccountSettings
 */
ImServerAccountSettings*
im_server_account_settings_new (void)
{
	return g_object_new (IM_TYPE_SERVER_ACCOUNT_SETTINGS, NULL);
}

/**
 * im_server_account_settings_get_hostname:
 * @settings: a #ImServerAccountSettings
 *
 * get the server hostname.
 *
 * Returns: a string
 */
const gchar* 
im_server_account_settings_get_hostname (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->hostname;
}

/**
 * im_server_account_settings_set_hostname:
 * @settings: a #ImServerAccountSettings
 * @hostname: a string.
 *
 * set @hostname as the server hostname.
 */
void         
im_server_account_settings_set_hostname (ImServerAccountSettings *settings,
					     const gchar *hostname)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->hostname);
	priv->hostname = g_strdup (hostname);

	g_object_notify (G_OBJECT (settings), "hostname");
}

/**
 * im_server_account_settings_get_uri:
 * @settings: a #ImServerAccountSettings
 *
 * get the uri, if any. If this is set, then all the other fields are invalid. It's only valid if protocol is %NULL.
 *
 * Returns: a string
 */
const gchar* 
im_server_account_settings_get_uri (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->uri;
}

/**
 * im_server_account_settings_set_uri:
 * @settings: a #ImServerAccountSettings
 * @uri: a string
 *
 * set @uri. When you set an @uri, then the protocol is set to %IM_PROTOCOL_REGISTRY_TYPE_INVALID. This is used for setting maildir or mbox
 * accounts.
 */
void         
im_server_account_settings_set_uri (ImServerAccountSettings *settings,
				    const gchar *uri)
{
	ImServerAccountSettingsPrivate *priv;
	
	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->uri);
	priv->uri = g_strdup (uri);

	g_object_notify (G_OBJECT (settings), "uri");
}

/**
 * im_server_account_settings_get_username:
 * @settings: a #ImServerAccountSettings
 *
 * get the username.
 *
 * Returns: a string
 */
const gchar* 
im_server_account_settings_get_username (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->username;
}

/**
 * im_server_account_settings_set_username:
 * @settings: a #ImServerAccountSettings
 * @username: a string
 *
 * set @username.
 */
void         
im_server_account_settings_set_username (ImServerAccountSettings *settings,
					 const gchar *username)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->username);
	priv->username = g_strdup (username);

	g_object_notify (G_OBJECT (settings), "username");
}

/**
 * im_server_account_settings_get_password:
 * @settings: a #ImServerAccountSettings
 *
 * get the password.
 *
 * Returns: a string
 */
const gchar* 
im_server_account_settings_get_password (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->password;
}

/**
 * im_server_account_settings_set_password:
 * @settings: a #ImServerAccountSettings
 * @password: a string
 *
 * set @password.
 */
void         
im_server_account_settings_set_password (ImServerAccountSettings *settings,
					 const gchar *password)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (priv->password) {
		bzero (priv->password, strlen (priv->password));
		g_free (priv->password);
	}
	priv->password = g_strdup (password);

	g_object_notify (G_OBJECT (settings), "password");
}

/**
 * im_server_account_settings_get_account_name:
 * @settings: a #ImServerAccountSettings
 *
 * get the #ImAccountMgr account name for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar* 
im_server_account_settings_get_account_name (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->account_name;
}

/**
 * im_server_account_settings_set_account_name:
 * @settings: a #ImServerAccountSettings
 * @account_name: a string
 *
 * sets the account name that will be used to store the account settings. This should
 * only be called from #ImAccountMgr and #ImAccountSettings.
 */
void         
im_server_account_settings_set_account_name (ImServerAccountSettings *settings,
						 const gchar *account_name)
{
	ImServerAccountSettingsPrivate *priv;

	/* be careful. This method should only be used internally in #ImAccountMgr and
	 * #ImAccountSettings. */

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);

	g_object_notify (G_OBJECT (settings), "account-name");
}

/**
 * im_server_account_settings_get_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the server protocol.
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType
im_server_account_settings_get_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->protocol;
}

/**
 * im_server_account_settings_set_protocol:
 * @settings: a #ImServerAccountSettings
 * @protocol: a #ImProtocolType
 *
 * set @server_type.
 */
void                          
im_server_account_settings_set_protocol (ImServerAccountSettings *settings,
					 ImProtocolType protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->protocol = protocol;
	
	g_object_notify (G_OBJECT (settings), "protocol");
	g_object_notify (G_OBJECT (settings), "protocol-name");
}

/**
 * im_server_account_settings_get_port:
 * @settings: a #ImServerAccountSettings
 *
 * get the server port.
 *
 * Returns: a #guint
 */
guint  
im_server_account_settings_get_port (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->port;
}

/**
 * im_server_account_settings_set_port:
 * @settings: a #ImServerAccountSettings
 * @port: a #guint.
 *
 * set @port.
 */
void   
im_server_account_settings_set_port (ImServerAccountSettings *settings,
				     guint port)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->port = port;

	g_object_notify (G_OBJECT (settings), "port");
}

/**
 * im_server_account_settings_get_security_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the secure connection type, if any.
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType
im_server_account_settings_get_security_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->security_protocol;
}

/**
 * im_server_account_settings_set_security_protocol:
 * @settings: a #ImServerAccountSettings
 * @security: a #ImProtocolType
 *
 * set the current security connection protocol to @security.
 */
void   
im_server_account_settings_set_security_protocol (ImServerAccountSettings *settings,
						  ImProtocolType security_protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->security_protocol = security_protocol;

	g_object_notify (G_OBJECT (settings), "security-protocol");
	g_object_notify (G_OBJECT (settings), "security-protocol-name");
}

/**
 * im_server_account_settings_get_auth_protocol:
 * @settings: a #ImServerAccountSettings
 *
 * get the authentication protocol
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType
im_server_account_settings_get_auth_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->auth_protocol;
}

/**
 * im_server_account_settings_set_auth_protocol:
 * @settings: a #ImServerAccountSettings
 * @auth_protocol: an #ImProtocolType
 *
 * set the current authentication protocol to @auth_protocol.
 */
void   
im_server_account_settings_set_auth_protocol (ImServerAccountSettings *settings,
					      ImProtocolType auth_protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->auth_protocol = auth_protocol;

	g_object_notify (G_OBJECT (settings), "auth-protocol");
	g_object_notify (G_OBJECT (settings), "auth-protocol-name");
}

