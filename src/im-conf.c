/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-conf.c : GConf settings wrapper */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Dirk-Jan C. Binnema
 *  Murray Cumming
 *  Philip Van Hoof
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

#include <config.h>
#include "im-conf.h"

#include <im-error.h>

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>

static void   im_conf_class_init     (ImConfClass *klass);

static void   im_conf_init           (ImConf *obj);

static void   im_conf_finalize       (GObject *obj);

static void   im_conf_on_change	 (GConfClient *client, guint conn_id,
				  GConfEntry *entry, gpointer data);

static GConfValueType im_conf_type_to_gconf_type (ImConfValueType value_type, 
						  GError **err);

/* list my signals */
enum {
	KEY_CHANGED_SIGNAL,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0}; 


typedef struct _ImConfPrivate ImConfPrivate;
struct _ImConfPrivate {
	GConfClient *gconf_client;
};
#define IM_CONF_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
								 IM_TYPE_CONF, \
								 ImConfPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
im_conf_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImConfClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) im_conf_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ImConf),
			1,		/* n_preallocs */
			(GInstanceInitFunc) im_conf_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ImConf",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
im_conf_class_init (ImConfClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = im_conf_finalize;

	g_type_class_add_private (gobject_class, sizeof(ImConfPrivate));
	
	signals[KEY_CHANGED_SIGNAL] =
		g_signal_new ("key_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ImConfClass,key_changed),
			      NULL, NULL,
			      g_cclosure_marshal_generic,
			      G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT); 
}

static void
im_conf_init (ImConf *obj)
{
	GConfClient *conf = NULL;
	GError *error = NULL;
	ImConfPrivate *priv = IM_CONF_GET_PRIVATE(obj);

	priv->gconf_client = NULL;
	
	conf = gconf_client_get_default ();
	if (!conf) {
		g_printerr ("im: could not get gconf client\n");
		return;
	}

	priv->gconf_client = conf;

	/* All the tree will be listened */
	gconf_client_add_dir (priv->gconf_client,
			      "/apps/im",
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);

	/* Notify every change under namespace */
	if (!error) {
		gconf_client_notify_add (priv->gconf_client,
					 "/apps/im",
					 im_conf_on_change,
					 obj,
					 NULL,
					 &error);
	} else {
		g_error_free (error);
	}
}

static void
im_conf_finalize (GObject *obj)
{
	ImConfPrivate *priv = IM_CONF_GET_PRIVATE(obj);
	if (priv->gconf_client) {

		gconf_client_suggest_sync (priv->gconf_client, NULL);

		g_object_unref (priv->gconf_client);
		priv->gconf_client = NULL;
	}	

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static ImConf*
im_conf_new (void)
{
	ImConf *conf;
	ImConfPrivate *priv;
	
	conf = IM_CONF(g_object_new(IM_TYPE_CONF, NULL));
	if (!conf) {
		g_printerr ("im: failed to init ImConf (GConf)\n");
		return NULL;
	}

	priv = IM_CONF_GET_PRIVATE(conf);
	if (!priv->gconf_client) {
		g_printerr ("im: failed to init gconf\n");
		g_object_unref (conf);
		return NULL;
	}
	
	return conf;
}

ImConf*
im_conf_get_instance (void)
{
	static ImConf *conf = 0;

	if (conf == 0) {
		conf = im_conf_new ();
	}

	return conf;
}


gchar*
im_conf_get_string (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (key,  NULL);

	priv = IM_CONF_GET_PRIVATE(self);
	return gconf_client_get_string (priv->gconf_client, key, err);
}


gint
im_conf_get_int (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (key, -1);

	priv = IM_CONF_GET_PRIVATE(self);
	
	return gconf_client_get_int (priv->gconf_client, key, err);
}

gdouble
im_conf_get_float (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (key, -1);

	priv = IM_CONF_GET_PRIVATE(self);
	
	return gconf_client_get_float (priv->gconf_client, key, err);
}

gboolean
im_conf_get_bool (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_CONF_GET_PRIVATE(self);
	
	return gconf_client_get_bool (priv->gconf_client, key, err);
}


GSList * 
im_conf_get_list (ImConf* self, const gchar* key, ImConfValueType list_type,
		      GError **err)
{
	ImConfPrivate *priv;
	GConfValueType gconf_type;
       
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (key,  NULL);

	priv = IM_CONF_GET_PRIVATE(self);

	gconf_type = im_conf_type_to_gconf_type (list_type, err);

	return gconf_client_get_list (priv->gconf_client, key, gconf_type, err);
}




gboolean
im_conf_set_string (ImConf* self, const gchar* key, const gchar* val,
			GError **err)
{
	ImConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	g_return_val_if_fail (val, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key,err)) {
		g_printerr ("im: '%s' is not writable\n", key);
		return FALSE;
	}
			
	return gconf_client_set_string (priv->gconf_client, key, val, err);
}

gboolean
im_conf_set_int  (ImConf* self, const gchar* key, gint val,
		      GError **err)
{
	ImConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key,err)) {
		g_printerr ("im: '%s' is not writable\n", key);
		return FALSE;
	}
			
	return gconf_client_set_int (priv->gconf_client, key, val, err);
}

gboolean
im_conf_set_float (ImConf* self, 
		       const gchar* key, 
		       gdouble val,
		       GError **err)
{
	ImConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key,err)) {
		g_printerr ("im: '%s' is not writable\n", key);
		return FALSE;
	}
			
	return gconf_client_set_float (priv->gconf_client, key, val, err);
}


gboolean
im_conf_set_bool (ImConf* self, const gchar* key, gboolean val,
		      GError **err)
{
	ImConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key, err)) {
		g_warning ("im: '%s' is not writable\n", key);
		return FALSE;
	}

	return gconf_client_set_bool (priv->gconf_client, key, val, err);
}


gboolean
im_conf_set_list (ImConf* self, const gchar* key, 
		      GSList *val, ImConfValueType list_type, 
		      GError **err)
{
	ImConfPrivate *priv;
	GConfValueType gconf_type;
       
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = IM_CONF_GET_PRIVATE(self);

	gconf_type = im_conf_type_to_gconf_type (list_type, err);

	return gconf_client_set_list (priv->gconf_client, key, gconf_type, val, err);
}


GSList*
im_conf_list_subkeys (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);
			
	return gconf_client_all_dirs (priv->gconf_client,key,err);
}


gboolean
im_conf_remove_key (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;
	gboolean retval;
	
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);
			
	retval = gconf_client_recursive_unset (priv->gconf_client,key,0,err);
	gconf_client_suggest_sync (priv->gconf_client, NULL);

	return retval;
}


gboolean
im_conf_key_exists (ImConf* self, const gchar* key, GError **err)
{
	ImConfPrivate *priv;
	GConfValue *val;

	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = IM_CONF_GET_PRIVATE(self);

	/* the fast way... */
	if (gconf_client_dir_exists (priv->gconf_client,key,err))
		return TRUE;
	
	val = gconf_client_get (priv->gconf_client, key, NULL);
	if (!val)
		return FALSE;
	else {
		gconf_value_free (val);
		return TRUE;
	}	
}


gchar*
im_conf_key_escape (const gchar* key)
{
	g_return_val_if_fail (key, NULL);
	g_return_val_if_fail (strlen (key) > 0, g_strdup (key));
	
	return gconf_escape_key (key, strlen(key));
}


gchar*
im_conf_key_unescape (const gchar* key)
{
	g_return_val_if_fail (key, NULL);

	return gconf_unescape_key (key, strlen(key));
}

gboolean
im_conf_key_is_valid (const gchar* key)
{
	return gconf_valid_key (key, NULL);
}

static void
im_conf_on_change (GConfClient *client,
		       guint conn_id,
		       GConfEntry *entry,
		       gpointer data)
{
	ImConfEvent event;
	const gchar* key;

	event = (entry->value) ? IM_CONF_EVENT_KEY_CHANGED : IM_CONF_EVENT_KEY_UNSET;
	key    = gconf_entry_get_key (entry);

	g_signal_emit (G_OBJECT(data),
		       signals[KEY_CHANGED_SIGNAL], 0,
		       key, event, conn_id);
}

static GConfValueType
im_conf_type_to_gconf_type (ImConfValueType value_type, GError **err)
{
	GConfValueType gconf_type;

	switch (value_type) {
	case IM_CONF_VALUE_INT:
		gconf_type = GCONF_VALUE_INT;
		break;
	case IM_CONF_VALUE_BOOL:
		gconf_type = GCONF_VALUE_BOOL;
		break;
	case IM_CONF_VALUE_FLOAT:
		gconf_type = GCONF_VALUE_FLOAT;
		break;
	case IM_CONF_VALUE_STRING:
		gconf_type = GCONF_VALUE_STRING;
		break;
	default:
		gconf_type = GCONF_VALUE_INVALID;
		g_printerr ("im: invalid list value type %d\n", value_type);
		*err = g_error_new_literal (IM_ERROR_DOMAIN, 
					    IM_ERROR_CONF_INVALID_VALUE, 
					    "invalid list value type");
	}
	return gconf_type;
}

void
im_conf_listen_to_namespace (ImConf *self,
				 const gchar *namespace)
{
	ImConfPrivate *priv;
	GError *error = NULL;

	g_return_if_fail (IM_IS_CONF (self));
	g_return_if_fail (namespace);
	
	priv = IM_CONF_GET_PRIVATE(self);

	/* Add the namespace to the list of the namespaces that will
	   be observed */
	gconf_client_add_dir (priv->gconf_client,
			      namespace,
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);
}

void 
im_conf_forget_namespace (ImConf *self,
			      const gchar *namespace)
{
	ImConfPrivate *priv;

	g_return_if_fail (IM_IS_CONF (self));
	g_return_if_fail (namespace);
	
	priv = IM_CONF_GET_PRIVATE(self);

	/* Remove the namespace to the list of the namespaces that will
	   be observed */
	gconf_client_remove_dir (priv->gconf_client, namespace, NULL);
}

static GHashTable *hash_namespace = NULL;
static gchar *im_namespace = NULL;

static void 
init_hash_namespace (void)
{
	const gchar *env_value;

	if (hash_namespace != NULL)
		return;

	hash_namespace = g_hash_table_new (g_str_hash, g_str_equal);

	env_value = g_getenv (IM_NAMESPACE_ENV);

	if (env_value == NULL || env_value[0] == '\0') {
		env_value = IM_DEFAULT_NAMESPACE;
	}
	im_namespace = g_strdup (env_value);
}

const gchar *
im_defs_namespace (const gchar *string)
{
	const gchar *ret_value;

	if (hash_namespace == NULL) init_hash_namespace ();

	if (string == NULL)
		return (const gchar *) im_namespace;

	ret_value = (const gchar *) g_hash_table_lookup (hash_namespace, string);
	if (ret_value == NULL) {
		ret_value = (const gchar *) g_strconcat (im_namespace, string, NULL);
		g_hash_table_insert (hash_namespace, (gpointer) string, (gpointer) ret_value);
	}
	return ret_value;
}

