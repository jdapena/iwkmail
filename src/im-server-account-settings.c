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

#include <im-protocol-registry.h>
#include <im-server-account-settings.h>
#include <strings.h>
#include <string.h>

/* 'private'/'protected' functions */
static void   im_server_account_settings_class_init (ImServerAccountSettingsClass *klass);
static void   im_server_account_settings_finalize   (GObject *obj);
static void   im_server_account_settings_instance_init (ImServerAccountSettings *obj);

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
	gobject_class->finalize = im_server_account_settings_finalize;

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

ImServerAccountSettings*
im_server_account_settings_new (void)
{
	return g_object_new (IM_TYPE_SERVER_ACCOUNT_SETTINGS, NULL);
}

const gchar* 
im_server_account_settings_get_hostname (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->hostname;
}

void         
im_server_account_settings_set_hostname (ImServerAccountSettings *settings,
					     const gchar *hostname)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->hostname);
	priv->hostname = g_strdup (hostname);
}

const gchar* 
im_server_account_settings_get_uri (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->uri;
}

void         
im_server_account_settings_set_uri (ImServerAccountSettings *settings,
					const gchar *uri)
{
	ImServerAccountSettingsPrivate *priv;
	
	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->uri);
	priv->uri = g_strdup (uri);

}

const gchar* 
im_server_account_settings_get_username (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->username;
}

void         
im_server_account_settings_set_username (ImServerAccountSettings *settings,
					     const gchar *username)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->username);
	priv->username = g_strdup (username);
}

const gchar* 
im_server_account_settings_get_password (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->password;
}

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
}

const gchar* 
im_server_account_settings_get_account_name (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->account_name;
}

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
}

ImProtocolType
im_server_account_settings_get_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->protocol;
}

void                          
im_server_account_settings_set_protocol (ImServerAccountSettings *settings,
					     ImProtocolType protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->protocol = protocol;
	
}

guint  
im_server_account_settings_get_port (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->port;
}

void   
im_server_account_settings_set_port (ImServerAccountSettings *settings,
					 guint port)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->port = port;
}

ImProtocolType
im_server_account_settings_get_security_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->security_protocol;
}

void   
im_server_account_settings_set_security_protocol (ImServerAccountSettings *settings,
						      ImProtocolType security_protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->security_protocol = security_protocol;
}

ImProtocolType
im_server_account_settings_get_auth_protocol (ImServerAccountSettings *settings)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings), IM_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->auth_protocol;
}

void   
im_server_account_settings_set_auth_protocol (ImServerAccountSettings *settings,
						  ImProtocolType auth_protocol)
{
	ImServerAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = IM_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->auth_protocol = auth_protocol;
}

