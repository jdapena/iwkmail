/* Based on ModestProtocol */

/* Copyright (c) 2008, Nokia Corporation
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

#include <im-protocol.h>

enum {
	PROP_0,
	PROP_NAME,
	PROP_DISPLAY_NAME,
	PROP_TYPE_ID
};

typedef struct _ImProtocolTranslation ImProtocolTranslation;
struct _ImProtocolTranslation {
	TranslationFunc translation_func;
	gpointer userdata;
	GDestroyNotify data_destroy_func;
};

typedef struct _ImProtocolPrivate ImProtocolPrivate;
struct _ImProtocolPrivate {
	ImProtocolType type_id;
	gchar *name;
	gchar *display_name;
	GHashTable *properties;
	GHashTable *translations;
};

/* 'private'/'protected' functions */
static void   im_protocol_class_init (ImProtocolClass *klass);
static void   im_protocol_finalize   (GObject *obj);
static void   im_protocol_get_property (GObject *obj,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void   im_protocol_set_property (GObject *obj,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void   im_protocol_instance_init (ImProtocol *obj);
static void   translation_free (ImProtocolTranslation *translation);

#define IM_PROTOCOL_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
									 IM_TYPE_PROTOCOL, \
									 ImProtocolPrivate))

/* globals */
static GObjectClass *parent_class = NULL;
static ImProtocolType next_type_id = 0;
static GMutex *next_type_id_mutex = NULL;

GType
im_protocol_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImProtocolClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_protocol_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImProtocol),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_protocol_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImProtocol",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_protocol_class_init (ImProtocolClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	next_type_id_mutex = g_mutex_new ();
	object_class->finalize = im_protocol_finalize;
	object_class->set_property = im_protocol_set_property;
	object_class->get_property = im_protocol_get_property;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      _("Protocol name"),
							      _("The unique name of the protocol"),
							      NULL,
							      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_DISPLAY_NAME,
					 g_param_spec_string ("display-name",
							       _("Display name"),
							       _("The name of the protocol that user will see"),
							       NULL,
							       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_TYPE_ID,
					 g_param_spec_int ("type",
							   _("Protocol id"),
							   _("Protocol type unique id"),
							   G_MININT, G_MAXINT, IM_PROTOCOL_TYPE_INVALID,
							   G_PARAM_READABLE));

	g_type_class_add_private (object_class,
				  sizeof(ImProtocolPrivate));
}

static void
translation_free (ImProtocolTranslation *translation)
{
	if (translation->data_destroy_func)
		translation->data_destroy_func (translation->userdata);
	translation->data_destroy_func = NULL;
	translation->userdata = NULL;
	translation->translation_func = NULL;
	g_slice_free (ImProtocolTranslation, translation);
}

static void
im_protocol_instance_init (ImProtocol *obj)
{
	ImProtocolPrivate *priv;

	priv = IM_PROTOCOL_GET_PRIVATE (obj);

	priv->name = NULL;
	priv->display_name = NULL;
	g_mutex_lock (next_type_id_mutex);
	priv->type_id = next_type_id;
	next_type_id++;
	g_mutex_unlock (next_type_id_mutex);

	priv->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, g_free);
	priv->translations = g_hash_table_new_full (g_str_hash, g_str_equal,
						    g_free, (GDestroyNotify) translation_free);
}

static void   
im_protocol_finalize   (GObject *obj)
{
	ImProtocol *settings = IM_PROTOCOL (obj);
	ImProtocolPrivate *priv = IM_PROTOCOL_GET_PRIVATE (settings);

	g_free (priv->name);
	priv->name = NULL;
	g_free (priv->display_name);
	priv->display_name = NULL;

	g_hash_table_unref (priv->properties);
	priv->properties = NULL;

	g_hash_table_unref (priv->translations);
	priv->translations = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void   
im_protocol_get_property (GObject *obj,
			      guint property_id,
			      GValue *value,
			      GParamSpec *pspec)
{
	ImProtocol *protocol = IM_PROTOCOL (obj);
	ImProtocolPrivate *priv = IM_PROTOCOL_GET_PRIVATE (protocol);

	switch (property_id) {
	case PROP_NAME:
		g_value_set_static_string (value, priv->name);
		break;
	case PROP_DISPLAY_NAME:
		g_value_set_static_string (value, priv->name);
		break;
	case PROP_TYPE_ID:
		g_value_set_int (value, priv->type_id);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

static void   
im_protocol_set_property (GObject *obj,
			      guint property_id,
			      const GValue *value,
			      GParamSpec *pspec)
{
	ImProtocol *protocol = IM_PROTOCOL (obj);

	switch (property_id) {
	case PROP_NAME:
		im_protocol_set_name (protocol, g_value_get_string (value));
		break;
	case PROP_DISPLAY_NAME:
		im_protocol_set_display_name (protocol, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}


ImProtocol*
im_protocol_new (const gchar *name, const gchar *display_name)
{
	return g_object_new (IM_TYPE_PROTOCOL, "display-name", display_name, "name", name, NULL);
}

const gchar* 
im_protocol_get_name (ImProtocol *self)
{
	ImProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), NULL);

	priv = IM_PROTOCOL_GET_PRIVATE (self);	
	return priv->name;
}

void         
im_protocol_set_name (ImProtocol *self,
			  const gchar *name)
{
	ImProtocolPrivate *priv;

	g_return_if_fail (IM_IS_PROTOCOL (self));

	priv = IM_PROTOCOL_GET_PRIVATE (self);
	g_free (priv->name);
	priv->name = g_strdup (name);
}

const gchar* 
im_protocol_get_display_name (ImProtocol *self)
{
	ImProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), NULL);

	priv = IM_PROTOCOL_GET_PRIVATE (self);	
	return priv->display_name;
}

void         
im_protocol_set_display_name (ImProtocol *self,
				  const gchar *display_name)
{
	ImProtocolPrivate *priv;

	g_return_if_fail (IM_IS_PROTOCOL (self));

	priv = IM_PROTOCOL_GET_PRIVATE (self);
	g_free (priv->display_name);
	priv->display_name = g_strdup (display_name);
}

ImProtocolType  
im_protocol_get_type_id (ImProtocol *self)
{
	ImProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), IM_PROTOCOL_TYPE_INVALID);

	priv = IM_PROTOCOL_GET_PRIVATE (self);
	return priv->type_id;
}

void
im_protocol_set (ImProtocol *self,
		     const gchar *key, 
		     const gchar *value)
{
	ImProtocolPrivate *priv;

	g_return_if_fail (IM_IS_PROTOCOL (self));

	priv = IM_PROTOCOL_GET_PRIVATE (self);

	g_hash_table_replace (priv->properties,g_strdup (key), g_strdup (value));
}

const gchar *
im_protocol_get (ImProtocol *self,
		     const gchar *key)
{
	ImProtocolPrivate *priv;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), NULL);

	priv = IM_PROTOCOL_GET_PRIVATE (self);

	return g_hash_table_lookup (priv->properties, key);
}

void
im_protocol_set_translation (ImProtocol *self,
				 const gchar *id,
				 TranslationFunc translation_func,
				 gpointer userdata,
				 GDestroyNotify data_destroy_func)
{
	ImProtocolPrivate *priv;
	ImProtocolTranslation *translation;

	g_return_if_fail (IM_IS_PROTOCOL (self));

	priv = IM_PROTOCOL_GET_PRIVATE (self);

	translation = g_slice_new0 (ImProtocolTranslation);
	translation->translation_func = translation_func;
	translation->userdata = userdata;
	translation->data_destroy_func = data_destroy_func;

	g_hash_table_replace (priv->translations, g_strdup(id), (gpointer) translation);
}

gchar *
im_protocol_get_translation (ImProtocol *self,
				 const gchar *id,
				 ...)
{
	va_list args;
	gchar *result;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), NULL);

	va_start (args, id);
	result = im_protocol_va_get_translation (self, id, args);
	va_end (args);

	return result;
}

gchar *
im_protocol_va_get_translation (ImProtocol *self,
				    const gchar *id,
				    va_list args)
{
	ImProtocolPrivate *priv;
	ImProtocolTranslation *translation;
	va_list dest;
	gchar *result;

	g_return_val_if_fail (IM_IS_PROTOCOL (self), NULL);

	priv = IM_PROTOCOL_GET_PRIVATE (self);

	translation = g_hash_table_lookup (priv->translations, id);
	if (translation == NULL)
		return NULL;
	g_return_val_if_fail (translation->translation_func != NULL, NULL);

	G_VA_COPY (dest, args);
	result = translation->translation_func (translation->userdata, dest);
	va_end (dest);

	return result;
}
