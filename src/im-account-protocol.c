/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-protocol.c : Account settings protocol */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2008, Nokia Corporation
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

#include "im-account-protocol.h"

enum {
	PROP_0,
	PROP_PORT,
	PROP_ALTERNATE_PORT,
	PROP_ACCOUNT_G_TYPE,
};

typedef struct _ImAccountProtocolPrivate ImAccountProtocolPrivate;
struct _ImAccountProtocolPrivate {
	guint port;
	guint alternate_port;
	GHashTable *account_options;
	GHashTable *custom_auth_mechs;
	GType account_g_type;
};

/* 'private'/'protected' functions */
static void   im_account_protocol_class_init (ImAccountProtocolClass *klass);
static void   im_account_protocol_finalize   (GObject *obj);
static void   im_account_protocol_get_property (GObject *obj,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void   im_account_protocol_set_property (GObject *obj,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void   im_account_protocol_instance_init (ImAccountProtocol *obj);

#define IM_ACCOUNT_PROTOCOL_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
									     IM_TYPE_ACCOUNT_PROTOCOL, \
									     ImAccountProtocolPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
im_account_protocol_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImAccountProtocolClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_account_protocol_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImAccountProtocol),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_account_protocol_instance_init,
			NULL
		};

		my_type = g_type_register_static (IM_TYPE_PROTOCOL,
						  "ImAccountProtocol",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_account_protocol_class_init (ImAccountProtocolClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = im_account_protocol_finalize;
	object_class->set_property = im_account_protocol_set_property;
	object_class->get_property = im_account_protocol_get_property;

	g_object_class_install_property (object_class,
					 PROP_PORT,
					 g_param_spec_uint ("port",
							   _("Standard port"),
							   _("The standard port for the protocol"),
							   0, G_MAXINT, 0,
							   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ALTERNATE_PORT,
					 g_param_spec_uint ("alternate-port",
							   _("Alternate port"),
							   _("The alternate port for the protocol (usually used in SSL)"),
							   0, G_MAXINT, 0,
							   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ACCOUNT_G_TYPE,
					 g_param_spec_gtype ("account-g-type",
							     _("Account factory GType"),
							     _("Account factory GType used for creating new instances."),
							     G_TYPE_OBJECT,
							     G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (object_class,
				  sizeof(ImAccountProtocolPrivate));

	/* Virtual methods */
}

static void
im_account_protocol_instance_init (ImAccountProtocol *obj)
{
	ImAccountProtocolPrivate *priv;

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (obj);

	priv->port = 0;
	priv->alternate_port = 0;
	priv->account_g_type = 0;
	priv->account_options = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
	priv->custom_auth_mechs = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

}

static void   
im_account_protocol_finalize   (GObject *obj)
{
	ImAccountProtocol *protocol = IM_ACCOUNT_PROTOCOL (obj);
	ImAccountProtocolPrivate *priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (protocol);

	if (priv->account_options)
		g_hash_table_destroy (priv->account_options);

	if (priv->custom_auth_mechs)
		g_hash_table_destroy (priv->custom_auth_mechs);
	priv->custom_auth_mechs = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void   
im_account_protocol_get_property (GObject *obj,
				      guint property_id,
				      GValue *value,
				      GParamSpec *pspec)
{
	ImAccountProtocol *protocol = IM_ACCOUNT_PROTOCOL (obj);
	ImAccountProtocolPrivate *priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (protocol);

	switch (property_id) {
	case PROP_PORT:
		g_value_set_uint (value, priv->port);
		break;
	case PROP_ALTERNATE_PORT:
		g_value_set_uint (value, priv->alternate_port);
		break;
	case PROP_ACCOUNT_G_TYPE:
		g_value_set_gtype (value, priv->account_g_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

static void   
im_account_protocol_set_property (GObject *obj,
				      guint property_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
	ImAccountProtocol *protocol = IM_ACCOUNT_PROTOCOL (obj);

	switch (property_id) {
	case PROP_PORT:
		im_account_protocol_set_port (protocol, g_value_get_uint (value));
		break;
	case PROP_ALTERNATE_PORT:
		im_account_protocol_set_alternate_port (protocol, g_value_get_uint (value));
		break;
	case PROP_ACCOUNT_G_TYPE:
		im_account_protocol_set_account_g_type (protocol, g_value_get_gtype (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}


ImProtocol*
im_account_protocol_new (const gchar *name, const gchar *display_name,
			     guint port, guint alternate_port,
			     GType account_g_type)
{
	return g_object_new (IM_TYPE_ACCOUNT_PROTOCOL, 
			     "display-name", display_name, "name", name, 
			     "port", port, "alternate-port", alternate_port,
			     "account-g-type", account_g_type,
			     NULL);
}

guint
im_account_protocol_get_port (ImAccountProtocol *self)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->port;
}

void         
im_account_protocol_set_port (ImAccountProtocol *self,
				  guint port)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->port = port;
}


guint
im_account_protocol_get_alternate_port (ImAccountProtocol *self)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->alternate_port;
}

void         
im_account_protocol_set_alternate_port (ImAccountProtocol *self,
					    guint alternate_port)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->alternate_port = alternate_port;
}

GType
im_account_protocol_get_account_g_type (ImAccountProtocol *self)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->account_g_type;
}

GList *
im_account_protocol_get_account_option_keys (ImAccountProtocol *self)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), NULL);
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);

	return g_hash_table_get_keys (priv->account_options);
}

const char *
im_account_protocol_get_account_option (ImAccountProtocol *self, const char *key)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), NULL);
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);

	return g_hash_table_lookup (priv->account_options, key);
}

void
im_account_protocol_set_account_option (ImAccountProtocol *self,
					const char *key, const char *value)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);

	g_hash_table_replace (priv->account_options, (gpointer) key, g_strdup (value));

}

gboolean
im_account_protocol_has_custom_secure_auth_mech (ImAccountProtocol *self, 
						     ImProtocolType auth_protocol_type)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), FALSE);
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	return g_hash_table_lookup_extended (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type), NULL, NULL);
}

const gchar *
im_account_protocol_get_custom_secure_auth_mech (ImAccountProtocol *self, 
						     ImProtocolType auth_protocol_type)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), NULL);
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	return (const gchar *) g_hash_table_lookup (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type));
}

void
im_account_protocol_set_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type, const gchar *secure_auth_mech)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	g_hash_table_replace (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type), g_strdup (secure_auth_mech));
}

void
im_account_protocol_unset_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));
	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	g_hash_table_remove (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type));
}


void         
im_account_protocol_set_account_g_type (ImAccountProtocol *self,
					    GType account_g_type)
{
	ImAccountProtocolPrivate *priv;

	g_return_if_fail (IM_IS_ACCOUNT_PROTOCOL (self));

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->account_g_type = account_g_type;
}

GObject *
im_account_protocol_create_account (ImAccountProtocol *self)
{
	ImAccountProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_ACCOUNT_PROTOCOL (self), NULL);

	priv = IM_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	if (priv->account_g_type > 0) {
		return g_object_new (priv->account_g_type, NULL);
	} else {
		return NULL;
	}
}

