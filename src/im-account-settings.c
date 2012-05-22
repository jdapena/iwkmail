/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-settings.c : in memory representation of an account settings */

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

#include <im-account-settings.h>
#include <im-enum-types.h>

enum {
	PROP_0,
	PROP_ID,
	PROP_DISPLAY_NAME,
	PROP_ENABLED,
	PROP_IS_DEFAULT,
	PROP_FULLNAME,
	PROP_EMAIL_ADDRESS,
	PROP_USE_SIGNATURE,
	PROP_SIGNATURE,
	PROP_RETRIEVE_TYPE,
	PROP_RETRIEVE_LIMIT,
	PROP_LEAVE_MESSAGES_ON_SERVER,
	PROP_STORE_SETTINGS,
	PROP_TRANSPORT_SETTINGS,
};

/* 'private'/'protected' functions */
static void   im_account_settings_class_init      (ImAccountSettingsClass *klass);
static void   im_account_settings_finalize        (GObject *obj);
static void   im_account_settings_get_property    (GObject *obj,
						   guint property_id,
						   GValue *value,
						   GParamSpec *pspec);
static void   im_account_settings_set_property    (GObject *obj,
						   guint property_id,
						   const GValue *value,
						   GParamSpec *pspec);
static void   im_account_settings_instance_init   (ImAccountSettings *obj);

typedef struct _ImAccountSettingsPrivate ImAccountSettingsPrivate;
struct _ImAccountSettingsPrivate {
	gchar *id;
	gchar *fullname;
	gchar *email_address;
	ImAccountRetrieveType retrieve_type;
	guint retrieve_limit;
	gchar *display_name;
	ImServerAccountSettings *store_settings;
	ImServerAccountSettings *transport_settings;
	gboolean enabled;
	gboolean is_default;
	gboolean leave_messages_on_server;
	gboolean use_signature;
	gchar *signature;
	gboolean use_connection_specific_smtp;
};

#define IM_ACCOUNT_SETTINGS_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
						    IM_TYPE_ACCOUNT_SETTINGS, \
						    ImAccountSettingsPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

/**
 * im_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType
im_account_settings_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImAccountSettingsClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_account_settings_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImAccountSettings),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_account_settings_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImAccountSettings",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_account_settings_class_init (ImAccountSettingsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->get_property = im_account_settings_get_property;
	gobject_class->set_property = im_account_settings_set_property;
	gobject_class->finalize = im_account_settings_finalize;

	g_object_class_install_property
		(gobject_class, PROP_ID,
		 g_param_spec_string ("id",
				      _("Account Id"),
				      _("Unique identifier of the account"),
				      NULL, /* default value */
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_DISPLAY_NAME,
		 g_param_spec_string ("display-name",
				      _("Display name"),
				      _("Visible name of the account"),
				      NULL, /* default value */
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_ENABLED,
		 g_param_spec_boolean ("enabled",
				       _("Enabled"),
				       _("Is account enabled"),
				       FALSE, /* default value */
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_IS_DEFAULT,
		 g_param_spec_boolean ("is-default",
				       _("Is default"),
				       _("Is the default account"),
				       FALSE, /* default value */
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_FULLNAME,
		 g_param_spec_string ("fullname",
				      _("Full name"),
				      _("Full name of the account user"),
				      NULL, /* default value */
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_EMAIL_ADDRESS,
		 g_param_spec_string ("email-address",
				      _("Email address"),
				      _("Email address of the account user"),
				      NULL, /* default value */
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_USE_SIGNATURE,
		 g_param_spec_boolean ("use-signature",
				       _("Use signature"),
				       _("Should use signature on composing mails"),
				       FALSE, /* default value */
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_SIGNATURE,
		 g_param_spec_string ("signature",
				      _("Signature"),
				      _("Signature"),
				      NULL, /* default value */
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_RETRIEVE_TYPE,
		 g_param_spec_enum ("retrieve-type",
				    _("Mode of retrieval"),
				    _("What will get retrieved"),
				    IM_TYPE_ACCOUNT_RETRIEVE_TYPE,
				    IM_ACCOUNT_RETRIEVE_HEADERS_ONLY,
				    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_RETRIEVE_LIMIT,
		 g_param_spec_uint ("retrieve-limit",
				    _("Limit of messages to retrieve"),
				    "Max  number of messages to be retrieved",
				    0,
				    (guint) -1,
				    0,
				    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_LEAVE_MESSAGES_ON_SERVER,
		 g_param_spec_boolean ("leave-messages-on-server",
				       _("Leave messages on server"),
				       _("Should leave messages on server (only for POP3)"),
				       FALSE, /* default value */
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_STORE_SETTINGS,
		 g_param_spec_object ("store-settings",
				      _("Store settings"),
				      _("Storage service settings"),
				      IM_TYPE_SERVER_ACCOUNT_SETTINGS,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property
		(gobject_class, PROP_TRANSPORT_SETTINGS,
		 g_param_spec_object ("transport-settings",
				      _("Transport settings"),
				      _("Transport service settings"),
				      IM_TYPE_SERVER_ACCOUNT_SETTINGS,
				      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (gobject_class,
				  sizeof(ImAccountSettingsPrivate));
}

static void
im_account_settings_instance_init (ImAccountSettings *obj)
{
	ImAccountSettingsPrivate *priv;

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (obj);

	priv->fullname = NULL;
	priv->email_address = NULL;
	priv->retrieve_type = IM_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	priv->retrieve_limit = 0;
	priv->display_name = NULL;
	priv->id = NULL;
	priv->store_settings = NULL;
	priv->transport_settings = NULL;
	priv->enabled = TRUE;
	priv->is_default = FALSE;
	priv->leave_messages_on_server = TRUE;
	priv->use_signature = FALSE;
	priv->signature = FALSE;
	priv->use_connection_specific_smtp = FALSE;
}

static void   
im_account_settings_finalize   (GObject *obj)
{
	ImAccountSettings *settings = IM_ACCOUNT_SETTINGS (obj);
	ImAccountSettingsPrivate *priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->fullname);
	priv->fullname = NULL;
	g_free (priv->email_address);
	priv->email_address = NULL;
	g_free (priv->display_name);
	priv->display_name = NULL;
	g_free (priv->id);
	priv->id = NULL;
	g_free (priv->signature);
	priv->signature = FALSE;
	if (priv->store_settings) {
		g_object_unref (priv->store_settings);
		priv->store_settings = NULL;
	}
	if (priv->transport_settings) {
		g_object_unref (priv->transport_settings);
		priv->transport_settings = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
im_account_settings_get_property (GObject *obj,
				  guint property_id,
				  GValue *value,
				  GParamSpec *pspec)
{
	ImAccountSettings *self = IM_ACCOUNT_SETTINGS (obj);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value,
				    im_account_settings_get_id (self));
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_string (value,
				    im_account_settings_get_display_name (self));
		break;
	case PROP_ENABLED:
		g_value_set_boolean (value,
				     im_account_settings_get_enabled (self));
		break;
	case PROP_IS_DEFAULT:
		g_value_set_boolean (value,
				     im_account_settings_get_is_default (self));
		break;
	case PROP_FULLNAME:
		g_value_set_string (value,
				    im_account_settings_get_fullname (self));
		break;
	case PROP_EMAIL_ADDRESS:
		g_value_set_string (value,
				    im_account_settings_get_email_address (self));
		break;
	case PROP_USE_SIGNATURE:
		g_value_set_boolean (value,
				     im_account_settings_get_use_signature (self));
		break;
	case PROP_SIGNATURE:
		g_value_set_string (value,
				    im_account_settings_get_signature (self));
		break;
	case PROP_RETRIEVE_TYPE:
		g_value_set_enum (value,
				  im_account_settings_get_retrieve_type (self));
		break;
	case PROP_RETRIEVE_LIMIT:
		g_value_set_uint (value,
				  im_account_settings_get_retrieve_limit (self));
		break;
	case PROP_LEAVE_MESSAGES_ON_SERVER:
		g_value_set_boolean (value,
				     im_account_settings_get_leave_messages_on_server (self));
		break;
	case PROP_STORE_SETTINGS:
		g_value_take_object (value,
				     im_account_settings_get_store_settings (self));
		break;
	case PROP_TRANSPORT_SETTINGS:
		g_value_take_object (value,
				     im_account_settings_get_transport_settings (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

static void
im_account_settings_set_property (GObject *obj,
				  guint property_id,
				  const GValue *value,
				  GParamSpec *pspec)
{
	ImAccountSettings *self = IM_ACCOUNT_SETTINGS (obj);

	switch (property_id) {
	case PROP_ID:
		im_account_settings_set_id (self,
					    g_value_get_string (value));
		break;
	case PROP_DISPLAY_NAME:
		im_account_settings_set_display_name (self,
						      g_value_get_string (value));
		break;
	case PROP_ENABLED:
		im_account_settings_set_enabled (self,
						  g_value_get_boolean (value));
		break;
	case PROP_IS_DEFAULT:
		im_account_settings_set_is_default (self,
						    g_value_get_boolean (value));
		break;
	case PROP_FULLNAME:
		im_account_settings_set_fullname (self,
						  g_value_get_string (value));
		break;
	case PROP_EMAIL_ADDRESS:
		im_account_settings_set_email_address (self,
						       g_value_get_string (value));
		break;
	case PROP_USE_SIGNATURE:
		im_account_settings_set_use_signature (self,
						       g_value_get_boolean (value));
		break;
	case PROP_SIGNATURE:
		im_account_settings_set_signature (self,
						   g_value_get_string (value));
		break;
	case PROP_RETRIEVE_TYPE:
		im_account_settings_set_retrieve_type (self,
						       g_value_get_enum (value));
		break;
	case PROP_RETRIEVE_LIMIT:
		im_account_settings_set_retrieve_limit (self,
							g_value_get_uint (value));
		break;
	case PROP_LEAVE_MESSAGES_ON_SERVER:
		im_account_settings_set_leave_messages_on_server (self,
								  g_value_get_boolean (value));
	case PROP_STORE_SETTINGS:
		im_account_settings_set_store_settings (self,
							g_value_get_object (value));
		break;
	case PROP_TRANSPORT_SETTINGS:
		im_account_settings_set_transport_settings (self,
							    g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

/**
 * im_account_settings_new:
 *
 * creates a new instance of #ImAccountSettings
 *
 * Returns: a #ImAccountSettings
 */
ImAccountSettings*
im_account_settings_new (void)
{
	return g_object_new (IM_TYPE_ACCOUNT_SETTINGS, NULL);
}

/**
 * im_account_settings_get_fullname:
 * @settings: a #ImAccountSettings
 *
 * get the user full name.
 *
 * Returns: a string
 */
const gchar* 
im_account_settings_get_fullname (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->fullname;
}

/**
 * im_account_settings_set_fullname:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @fullname as the user full name .
 */
void         
im_account_settings_set_fullname (ImAccountSettings *settings,
				  const gchar *fullname)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->fullname);
	priv->fullname = g_strdup (fullname);

	g_object_notify (G_OBJECT (settings), "fullname");
}

/**
 * im_account_settings_get_email_address:
 * @settings: a #ImAccountSettings
 *
 * get the user email address.
 *
 * Returns: a string
 */
const gchar* 
im_account_settings_get_email_address (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->email_address;
}

/**
 * im_account_settings_set_email_address:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @email_address of the account.
 */
void         
im_account_settings_set_email_address (ImAccountSettings *settings,
				       const gchar *email_address)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->email_address);
	priv->email_address = g_strdup (email_address);

	g_object_notify (G_OBJECT (settings), "email-address");
}

/**
 * im_account_settings_get_display_name:
 * @settings: a #ImAccountSettings
 *
 * get the visible name of the account.
 *
 * Returns: a string
 */
const gchar* 
im_account_settings_get_display_name (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->display_name;
}

/**
 * im_account_settings_set_display_name:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @display_name as the name of the account visible to the users in UI.
 */
void         
im_account_settings_set_display_name (ImAccountSettings *settings,
					     const gchar *display_name)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->display_name);
	priv->display_name = g_strdup (display_name);

	g_object_notify (G_OBJECT (settings), "display-name");
}

/**
 * im_account_settings_get_id:
 * @settings: a #ImAccountSettings
 *
 * get the #ImAccountMgr account id for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar* 
im_account_settings_get_id (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->id;
}

/**
 * im_account_settings_set_id:
 * @settings: a #ImAccountSettings
 * @account_name: a string
 *
 * sets the account id that will be used to store the account settings. This should
 * only be called from #ImAccountMgr and #ImAccountSettings.
 */
void         
im_account_settings_set_id (ImAccountSettings *settings,
			    const gchar *id)
{
	ImAccountSettingsPrivate *priv;

	/* be careful. This method should only be used internally in #ImAccountMgr and
	 * #ImAccountSettings. */

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->id);
	priv->id = g_strdup (id);

	g_object_notify (G_OBJECT (settings), "id");
}

/**
 * im_account_settings_get_retrieve_type:
 * @settings: a #ImAccountSettings
 *
 * get the account retrieve type.
 *
 * Returns: a #ImAccountRetrieveType
 */
ImAccountRetrieveType  
im_account_settings_get_retrieve_type (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), IM_ACCOUNT_RETRIEVE_HEADERS_ONLY);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_type;
}

/**
 * im_account_settings_set_retrieve_type:
 * @settings: a #ImAccountSettings
 * @retrieve_type: a #ImAccountRetrieveType.
 *
 * set @retrieve_type of the account.
 */
void                          
im_account_settings_set_retrieve_type (ImAccountSettings *settings,
					   ImAccountRetrieveType retrieve_type)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_type = retrieve_type;

	g_object_notify (G_OBJECT (settings), "retrieve-type");
}

/**
 * im_account_settings_get_retrieve_limit:
 * @settings: a #ImAccountSettings
 *
 * get the account retrieve limit. 0 is no limit.
 *
 * Returns: a #gint
 */
guint  
im_account_settings_get_retrieve_limit (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_limit;
}

/**
 * im_account_settings_set_retrieve_limit:
 * @settings: a #ImAccountSettings
 * @retrieve_limit: a #gint.
 *
 * set @retrieve_limit of the account. 0 is no limit.
 */
void   
im_account_settings_set_retrieve_limit (ImAccountSettings *settings,
					guint retrieve_limit)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_limit = retrieve_limit;

	g_object_notify (G_OBJECT (settings), "retrieve-limit");
}

/**
 * im_account_settings_get_enabled:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the account is enabled or not.
 *
 * Returns: a #gboolean
 */
gboolean 
im_account_settings_get_enabled (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->enabled;
}

/**
 * im_account_settings_set_enabled:
 * @settings: a #ImAccountSettings
 * @enabled: a #gboolean
 *
 * set if @settings account is enabled or not.
 */
void   
im_account_settings_set_enabled (ImAccountSettings *settings,
				     gboolean enabled)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->enabled = enabled;

	g_object_notify (G_OBJECT (settings), "enabled");
}

/**
 * im_account_settings_get_is_default:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the account is the default account or not.
 *
 * Returns: a #gboolean
 */
gboolean 
im_account_settings_get_is_default (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->is_default;
}

/**
 * im_account_settings_set_is_default:
 * @settings: a #ImAccountSettings
 * @is_default: a #gboolean
 *
 * set if @settings account is the default account or not.
 */
void   
im_account_settings_set_is_default (ImAccountSettings *settings,
				     gboolean is_default)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->is_default = is_default;

	g_object_notify (G_OBJECT (settings), "is-default");
}

/**
 * im_account_settings_get_store_settings:
 * @settings: a #ImAccountSettings
 *
 * obtains a ref'ed instance of the store account server settings
 *
 * Returns: (transfer full): a ref'd #ImServerAccountSettings. You should unreference it on finishing usage.
 */
ImServerAccountSettings * 
im_account_settings_get_store_settings (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (!priv->store_settings)
		priv->store_settings = im_server_account_settings_new ();
	return g_object_ref (priv->store_settings);
}

/**
 * im_account_settings_set_store_settings:
 * @settings: a #ImAccountSettings
 *
 * sets @store_settings as the settings of the store account of @settings account.
 * @settings will keep an internal reference to it.
 */
void
im_account_settings_set_store_settings (ImAccountSettings *settings,
					    ImServerAccountSettings *store_settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);

	if (priv->store_settings) {
		g_object_unref (priv->store_settings);
		priv->store_settings = NULL;
	}

	if (IM_IS_SERVER_ACCOUNT_SETTINGS (store_settings))
		priv->store_settings = g_object_ref (store_settings);

	g_object_notify (G_OBJECT (settings), "store-settings");
}

/**
 * im_account_settings_get_transport_settings:
 * @settings: a #ImAccountSettings
 *
 * obtains a ref'ed instance of the transport account server settings
 *
 * Returns: (transfer full): a ref'd #ImServerAccountSettings. You should unreference it on finishing usage.
 */
ImServerAccountSettings * 
im_account_settings_get_transport_settings (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (!priv->transport_settings)
		priv->transport_settings = im_server_account_settings_new ();
	return g_object_ref (priv->transport_settings);
}

/**
 * im_account_settings_set_transport_settings:
 * @settings: a #ImAccountSettings
 *
 * sets @transport_settings as the settings of the transport account of @settings account.
 * @settings will keep an internal reference to it.
 */
void
im_account_settings_set_transport_settings (ImAccountSettings *settings,
					    ImServerAccountSettings *transport_settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);

	if (priv->transport_settings) {
		g_object_unref (priv->transport_settings);
		priv->transport_settings = NULL;
	}

	if (IM_IS_SERVER_ACCOUNT_SETTINGS (transport_settings))
		priv->transport_settings = g_object_ref (transport_settings);

	g_object_notify (G_OBJECT (settings), "transport-settings");
}

/**
 * im_account_settings_get_use_signature:
 * @settings: a #ImAccountSettings
 *
 * obtains whether the mails from this account use signature or not.
 *
 * Returns: a #gboolean
 */
gboolean 
im_account_settings_get_use_signature (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->use_signature;
}

/**
 * im_account_settings_set_use_signature:
 * @settings: a #ImAccountSettings
 * @use_signature: a #gboolean
 *
 * set if @settings mails use signature or not
 */
void   
im_account_settings_set_use_signature (ImAccountSettings *settings,
				     gboolean use_signature)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->use_signature = use_signature;

	g_object_notify (G_OBJECT (settings), "use-signature");
}

/**
 * im_account_settings_get_signature:
 * @settings: a #ImAccountSettings
 *
 * get the signature.
 *
 * Returns: a string
 */
const gchar* 
im_account_settings_get_signature (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->signature;
}

/**
 * im_account_settings_set_signature:
 * @settings: a #ImAccountSettings
 * @hostname: a string.
 *
 * set @signature for the account .
 */
void         
im_account_settings_set_signature (ImAccountSettings *settings,
					     const gchar *signature)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->signature);
	priv->signature = g_strdup (signature);

	g_object_notify (G_OBJECT (settings), "signature");
}

/**
 * im_account_settings_get_leave_messages_on_server:
 * @settings: a #ImAccountSettings
 *
 * obtains whether messages should be left on server or not
 *
 * Returns: a #gboolean
 */
gboolean 
im_account_settings_get_leave_messages_on_server (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->leave_messages_on_server;
}

/**
 * im_account_settings_set_leave_messages_on_server:
 * @settings: a #ImAccountSettings
 * @leave_messages_on_server: a #gboolean
 *
 * set if we leave the messages on server or not.
 */
void
im_account_settings_set_leave_messages_on_server (ImAccountSettings *settings,
						  gboolean leave_messages_on_server)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->leave_messages_on_server = leave_messages_on_server;

	g_object_notify (G_OBJECT (settings), "leave-messages-on-server");
}

