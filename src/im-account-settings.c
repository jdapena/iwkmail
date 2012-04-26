/* Based on ModestAccountSettings */

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

#include <im-account-settings.h>

/* 'private'/'protected' functions */
static void   im_account_settings_class_init (ImAccountSettingsClass *klass);
static void   im_account_settings_finalize   (GObject *obj);
static void   im_account_settings_instance_init (ImAccountSettings *obj);

typedef struct _ImAccountSettingsPrivate ImAccountSettingsPrivate;
struct _ImAccountSettingsPrivate {
	gchar *fullname;
	gchar *email_address;
	ImAccountRetrieveType retrieve_type;
	gint retrieve_limit;
	gchar *display_name;
	gchar *account_name;
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
	gobject_class->finalize = im_account_settings_finalize;

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
	priv->account_name = NULL;
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
	g_free (priv->account_name);
	priv->account_name = NULL;
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

ImAccountSettings*
im_account_settings_new (void)
{
	return g_object_new (IM_TYPE_ACCOUNT_SETTINGS, NULL);
}

const gchar* 
im_account_settings_get_fullname (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->fullname;
}

void         
im_account_settings_set_fullname (ImAccountSettings *settings,
					     const gchar *fullname)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->fullname);
	priv->fullname = g_strdup (fullname);
}

const gchar* 
im_account_settings_get_email_address (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->email_address;
}

void         
im_account_settings_set_email_address (ImAccountSettings *settings,
					     const gchar *email_address)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->email_address);
	priv->email_address = g_strdup (email_address);
}

const gchar* 
im_account_settings_get_display_name (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->display_name;
}

void         
im_account_settings_set_display_name (ImAccountSettings *settings,
					     const gchar *display_name)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->display_name);
	priv->display_name = g_strdup (display_name);
}

const gchar* 
im_account_settings_get_account_name (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->account_name;
}

void         
im_account_settings_set_account_name (ImAccountSettings *settings,
						 const gchar *account_name)
{
	ImAccountSettingsPrivate *priv;

	/* be careful. This method should only be used internally in #ImAccountMgr and
	 * #ImAccountSettings. */

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);
}

ImAccountRetrieveType  
im_account_settings_get_retrieve_type (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), IM_ACCOUNT_RETRIEVE_HEADERS_ONLY);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_type;
}

void                          
im_account_settings_set_retrieve_type (ImAccountSettings *settings,
					   ImAccountRetrieveType retrieve_type)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_type = retrieve_type;
}

gint  
im_account_settings_get_retrieve_limit (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_limit;
}

void   
im_account_settings_set_retrieve_limit (ImAccountSettings *settings,
					    gint retrieve_limit)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_limit = retrieve_limit;
}

gboolean 
im_account_settings_get_enabled (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->enabled;
}

void   
im_account_settings_set_enabled (ImAccountSettings *settings,
				     gboolean enabled)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->enabled = enabled;
}

gboolean 
im_account_settings_get_is_default (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->is_default;
}

void   
im_account_settings_set_is_default (ImAccountSettings *settings,
				     gboolean is_default)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->is_default = is_default;
}

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
}

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
}

gboolean 
im_account_settings_get_use_signature (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->use_signature;
}

void   
im_account_settings_set_use_signature (ImAccountSettings *settings,
				     gboolean use_signature)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->use_signature = use_signature;
}

const gchar* 
im_account_settings_get_signature (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->signature;
}

void         
im_account_settings_set_signature (ImAccountSettings *settings,
					     const gchar *signature)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->signature);
	priv->signature = g_strdup (signature);
}

gboolean 
im_account_settings_get_leave_messages_on_server (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->leave_messages_on_server;
}

void   
im_account_settings_set_leave_messages_on_server (ImAccountSettings *settings,
				     gboolean leave_messages_on_server)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->leave_messages_on_server = leave_messages_on_server;
}

gboolean 
im_account_settings_get_use_connection_specific_smtp (ImAccountSettings *settings)
{
	ImAccountSettingsPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->use_connection_specific_smtp;
}

void   
im_account_settings_set_use_connection_specific_smtp (ImAccountSettings *settings,
							  gboolean use_connection_specific_smtp)
{
	ImAccountSettingsPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_SETTINGS (settings));

	priv = IM_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->use_connection_specific_smtp = use_connection_specific_smtp;
}

