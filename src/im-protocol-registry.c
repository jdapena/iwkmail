/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-protocol-registry.c : Registry of protocols available */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
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

#include <string.h>
#include <im-account-protocol.h>
#include <im-protocol-registry.h>

#define TAG_ALL_PROTOCOLS "__IM_PROTOCOL_REGISTRY_ALL_PROTOCOLS"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-store.c 
 */
#define IM_ACCOUNT_OPTION_SSL_NEVER "never"
/* This is a tinymail camel-lite specific option, 
 * roughly equivalent to "always" in regular camel,
 * which is appropriate for a generic "SSL" connection option: */
#define IM_ACCOUNT_OPTION_SSL_WRAPPED "wrapped"
/* Not used in our UI so far: */
#define IM_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"
/* This is a tinymailcamel-lite specific option that is not in regular camel. */
#define IM_ACCOUNT_OPTION_SSL_TLS "tls"

/* Posssible values for tny_account_set_secure_auth_mech().
 * These might be camel-specific.
 * Really, tinymail should use an enum.
 * camel_sasl_authtype() seems to list some possible values.
 */
 
/* Note that evolution does not offer these for IMAP: */
#define IM_ACCOUNT_AUTH_PLAIN "PLAIN"
#define IM_ACCOUNT_AUTH_ANONYMOUS "ANONYMOUS"

/* Caeml's IMAP uses NULL instead for "Password".
 * Also, not that Evolution offers "Password" for IMAP, but "Login" for SMTP.*/
#define IM_ACCOUNT_AUTH_PASSWORD "LOGIN" 
#define IM_ACCOUNT_AUTH_CRAMMD5 "CRAM-MD5"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-provider.c 
 */
#define IM_ACCOUNT_OPTION_USE_LSUB "use_lsub" /* Show only subscribed folders */
#define IM_ACCOUNT_OPTION_CHECK_ALL "check_all" /* Check for new messages in all folders */

/* 'private'/'protected' functions */
static void   im_protocol_registry_class_init (ImProtocolRegistryClass *klass);
static void   im_protocol_registry_finalize   (GObject *obj);
static void   im_protocol_registry_instance_init (ImProtocolRegistry *obj);
static GHashTable *   im_protocol_registry_create_tag (ImProtocolRegistry *obj, const gchar *tag);

/* translation handlers */
static gchar * translation_is_userdata (gpointer userdata, va_list args);

typedef struct _ImProtocolRegistryPrivate ImProtocolRegistryPrivate;
struct _ImProtocolRegistryPrivate {
	GHashTable *tags_table;
	GHashTable *priorities;
};

#define IM_PROTOCOL_REGISTRY_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
									 IM_TYPE_PROTOCOL_REGISTRY, \
									 ImProtocolRegistryPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

static ImProtocolType pop_protocol_type_id = -1;
static ImProtocolType imap_protocol_type_id = -1;
static ImProtocolType maildir_protocol_type_id = -1;
static ImProtocolType mbox_protocol_type_id = -1;
static ImProtocolType smtp_protocol_type_id = -1;
static ImProtocolType sendmail_protocol_type_id = -1;
static ImProtocolType none_connection_protocol_type_id = -1;
static ImProtocolType ssl_connection_protocol_type_id = -1;
static ImProtocolType tls_connection_protocol_type_id = -1;
static ImProtocolType tlsop_connection_protocol_type_id = -1;
static ImProtocolType none_auth_protocol_type_id = -1;
static ImProtocolType password_auth_protocol_type_id = -1;
static ImProtocolType crammd5_auth_protocol_type_id = -1;

GType
im_protocol_registry_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImProtocolRegistryClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_protocol_registry_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImProtocolRegistry),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_protocol_registry_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImProtocolRegistry",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_protocol_registry_class_init (ImProtocolRegistryClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = im_protocol_registry_finalize;
	g_type_class_add_private (object_class,
				  sizeof(ImProtocolRegistryPrivate));
}

static void
im_protocol_registry_instance_init (ImProtocolRegistry *obj)
{
	ImProtocolRegistryPrivate *priv;

	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (obj);

	priv->tags_table = g_hash_table_new_full (g_str_hash, g_str_equal, 
						  g_free, (GDestroyNotify) g_hash_table_unref);
	priv->priorities = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	im_protocol_registry_create_tag (obj, TAG_ALL_PROTOCOLS);
}

static void   
im_protocol_registry_finalize   (GObject *obj)
{
	ImProtocolRegistryPrivate *priv;

	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (obj);

	/* Free hash tables */
	if (priv->tags_table)
		g_hash_table_unref (priv->tags_table);
	if (priv->priorities)
		g_hash_table_unref (priv->priorities);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


ImProtocolRegistry*
im_protocol_registry_get_instance ()
{
	static ImProtocolRegistry *instance = 0;

	if (instance == 0) {
		instance = g_object_new (IM_TYPE_PROTOCOL_REGISTRY, NULL);
		im_protocol_registry_set_to_default (instance);
	}

	return instance;
}

void
im_protocol_registry_add (ImProtocolRegistry *self, ImProtocol *protocol, gint priority, const gchar *first_tag, ...)
{
	va_list list;
	GSList *tags_list = NULL, *node = NULL;
	const gchar *va_string;
	GHashTable *tag_table;
	ImProtocolRegistryPrivate *priv;

	g_return_if_fail (IM_IS_PROTOCOL_REGISTRY (self));
	g_return_if_fail (IM_IS_PROTOCOL (protocol));
	g_return_if_fail (first_tag != NULL);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, TAG_ALL_PROTOCOLS);
	g_hash_table_insert (tag_table, GINT_TO_POINTER (im_protocol_get_type_id (protocol)), g_object_ref (protocol));

	g_hash_table_insert (priv->priorities, GINT_TO_POINTER (im_protocol_get_type_id (protocol)), GINT_TO_POINTER (priority));

	tags_list = g_slist_prepend (tags_list, (gpointer) first_tag);
	va_start (list, first_tag);
	while ((va_string = va_arg (list, const gchar *)) != NULL) {
		tags_list = g_slist_prepend (tags_list, (gpointer) va_string);
	}
	va_end (list);

	for (node = tags_list; node != NULL; node = g_slist_next (node)) {

		tag_table = g_hash_table_lookup (priv->tags_table, node->data);
		if (tag_table == NULL)
			tag_table = im_protocol_registry_create_tag (self, node->data);
		g_hash_table_insert (tag_table, GINT_TO_POINTER (im_protocol_get_type_id (protocol)), g_object_ref (protocol));
	}
	g_slist_free (tags_list);
}

GSList *
im_protocol_registry_get_all (ImProtocolRegistry *self)
{
	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), NULL);

	return im_protocol_registry_get_by_tag (self, TAG_ALL_PROTOCOLS);
	
}

static void
add_protocol_to_list (ImProtocolType key, ImProtocol *protocol, GSList **list)
{
	*list = g_slist_prepend (*list, protocol);
}

static gint
compare_protocols (ImProtocol *a, ImProtocol *b, GHashTable *priorities)
{
	ImProtocolType a_type, b_type;
	gint result;

	a_type = im_protocol_get_type_id (a);
	b_type = im_protocol_get_type_id (b);

	result = GPOINTER_TO_INT (g_hash_table_lookup (priorities, GINT_TO_POINTER (a_type))) - GPOINTER_TO_INT (g_hash_table_lookup (priorities, GINT_TO_POINTER (b_type)));
	if (result == 0) {
		const gchar *a_display_name;
		const gchar *b_display_name;

		a_display_name = im_protocol_get_display_name (a);
		b_display_name = im_protocol_get_display_name (b);
		result = g_utf8_collate (a_display_name, b_display_name);
	}
	return result;
}

GSList *
im_protocol_registry_get_by_tag (ImProtocolRegistry *self, const gchar *tag)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;
	GSList *result;

	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}

	result = NULL;
	g_hash_table_foreach (tag_table, (GHFunc) add_protocol_to_list, &result);

	result = g_slist_sort_with_data (result, (GCompareDataFunc) compare_protocols, priv->priorities);

	return result;

}

static void
add_protocol_to_pair_list (ImProtocolType type_id, ImProtocol *protocol, GSList **list)
{
	*list = g_slist_append (*list,
				(gpointer) im_pair_new (
					(gpointer) im_protocol_get_name (protocol),
					(gpointer) im_protocol_get_display_name (protocol),
					FALSE));
}

ImPairList *
im_protocol_registry_get_pair_list_by_tag (ImProtocolRegistry *self, const gchar *tag)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;
	GSList *result = NULL;

	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}

	g_hash_table_foreach (tag_table, (GHFunc) add_protocol_to_pair_list, &result);

	return result;
}

static gboolean
find_protocol_by_name (ImProtocolType type_id,
		       ImProtocol *protocol,
		       const gchar *name)
{
	return (strcmp (name, im_protocol_get_name (protocol)) == 0);
}

ImProtocol *
im_protocol_registry_get_protocol_by_name (ImProtocolRegistry *self, 
					       const gchar *tag, 
					       const gchar *name)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), NULL);
	g_return_val_if_fail (tag, NULL);
	g_return_val_if_fail (name, NULL);

	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}
	
	return g_hash_table_find (tag_table, (GHRFunc) find_protocol_by_name, (gpointer) name);
}

ImProtocol *
im_protocol_registry_get_protocol_by_type (ImProtocolRegistry *self, ImProtocolType type_id)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, TAG_ALL_PROTOCOLS);
	if (tag_table == NULL) {
		return NULL;
	}
	
	return g_hash_table_lookup (tag_table, GINT_TO_POINTER (type_id));
}

gboolean 
im_protocol_registry_protocol_type_has_tag (ImProtocolRegistry *self, ImProtocolType type_id, const gchar *tag)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (IM_IS_PROTOCOL_REGISTRY (self), FALSE);
	g_return_val_if_fail (tag != NULL, FALSE);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return FALSE;
	}
	
	return (g_hash_table_lookup (tag_table, GINT_TO_POINTER (type_id))!= NULL);
	
}

static GHashTable *
im_protocol_registry_create_tag (ImProtocolRegistry *self, const gchar *tag)
{
	ImProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_assert (tag != NULL);
	priv = IM_PROTOCOL_REGISTRY_GET_PRIVATE (self);
	tag_table = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
					   NULL, g_object_unref);
	g_hash_table_insert (priv->tags_table, g_strdup (tag), tag_table);

	return tag_table;
}

static gchar * 
translation_is_userdata (gpointer userdata, va_list args)
{
	va_list dest;
	gchar *result;

	G_VA_COPY (dest, args);
	result = g_strdup_vprintf (_(userdata), dest);
	va_end (dest);

	return result;
}

static gchar * 
translation_is_userdata_no_param (gpointer userdata, va_list args)
{
	gchar *result;

	result = g_strdup (_(userdata));

	return result;
}


void 
im_protocol_registry_set_to_default (ImProtocolRegistry *self)
{
	ImProtocol *protocol;

	protocol = im_account_protocol_new ("sendmail", _("Sendmail"),
					    0, 0);
	sendmail_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_registry_add (self, protocol, 1,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
				      0);
	g_object_unref (protocol);

	protocol = im_account_protocol_new ("smtp", _("SMTP Server"),
					    25, 465);
	smtp_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, N_("Error connecting to SMTP server"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, N_("Error connecting to SMTP server"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, N_("SMTP authentication error"), NULL);
	im_protocol_registry_add (self, protocol, 2,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_account_protocol_new ("pop", _("POP3"),
					    110, 995);
	pop_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, N_("Error connecting to POP3 server"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, N_("POP3 authentication error"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, N_("Error connecting to POP3 server"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, translation_is_userdata_no_param, N_("Error fetching message"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER, translation_is_userdata, N_("Error fetching message header"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_SSL_PROTO_NAME, translation_is_userdata_no_param, N_("pop3s"), NULL);
	im_protocol_registry_add (self, protocol, 3,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_account_protocol_new ("imap", _("IMAP"),
					    143, 993);
	imap_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_DELETE_MAILBOX, translation_is_userdata, N_("Delete mailbox"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, N_("IMAP connection error"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, N_("IMAP authentication error"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, N_("Couldn't connect to server"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, translation_is_userdata, N_("Couldn't fetch message"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER, translation_is_userdata, N_("Couldn't fetch message header"), NULL);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_SSL_PROTO_NAME, translation_is_userdata_no_param, N_("imap4s"), NULL);
	im_account_protocol_set_account_option (IM_ACCOUNT_PROTOCOL (protocol),
						IM_ACCOUNT_OPTION_USE_LSUB, "");
	im_account_protocol_set_account_option (IM_ACCOUNT_PROTOCOL (protocol),
						IM_ACCOUNT_OPTION_CHECK_ALL, "");
	im_protocol_registry_add (self, protocol, 4,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_account_protocol_new ("maildir", _("Maildir"),
					    0, 0);
	maildir_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, 
				     translation_is_userdata_no_param, N_("Mailbox not available"), NULL);
	im_protocol_registry_add (self, protocol, 5, 
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_account_protocol_new ("mbox", _("MBox"),
					    0, 0);
	mbox_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set_translation (protocol, IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, 
				     translation_is_userdata_no_param, N_("Mailbox not available"), NULL);
	im_protocol_registry_add (self, protocol, 6,
				      IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new ("none", _("None"));
	im_protocol_set (protocol, IM_PROTOCOL_SECURITY_ACCOUNT_OPTION, IM_ACCOUNT_OPTION_SSL_NEVER);
	none_connection_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_registry_add (self, protocol, 7,
				      IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new ("ssl", _("SSL"));
	im_protocol_set (protocol, IM_PROTOCOL_SECURITY_ACCOUNT_OPTION, IM_ACCOUNT_OPTION_SSL_WRAPPED);
	ssl_connection_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_registry_add (self, protocol, 8,
				      IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new ("tls", _("TLS"));
	im_protocol_set (protocol, IM_PROTOCOL_SECURITY_ACCOUNT_OPTION, IM_ACCOUNT_OPTION_SSL_TLS);
	tls_connection_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_registry_add (self, protocol, 9, 
				      IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new ("tls-op", _("TLS when possible"));
	im_protocol_set (protocol, IM_PROTOCOL_SECURITY_ACCOUNT_OPTION, IM_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE);
	tlsop_connection_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_registry_add (self, protocol, 10,
				      IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new (IM_ACCOUNT_AUTH_MECH_VALUE_NONE, _("None"));
	none_auth_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set (protocol, IM_PROTOCOL_AUTH_ACCOUNT_OPTION, IM_ACCOUNT_AUTH_PLAIN);
	im_protocol_registry_add (self, protocol, 11,
				      IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new (IM_ACCOUNT_AUTH_MECH_VALUE_LOGIN, _("Login"));
	password_auth_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set (protocol, IM_PROTOCOL_AUTH_ACCOUNT_OPTION, IM_ACCOUNT_AUTH_PASSWORD);
	im_protocol_registry_add (self, protocol, 12,
				      IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = im_protocol_new (IM_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5, _("CRAM-MD5"));
	crammd5_auth_protocol_type_id = im_protocol_get_type_id (protocol);
	im_protocol_set (protocol, IM_PROTOCOL_AUTH_ACCOUNT_OPTION, IM_ACCOUNT_AUTH_CRAMMD5);
	im_protocol_registry_add (self, protocol, 13,
				      IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	/* set the custom auth mechs. We do this after creating all the protocols, because if we don't, then we
	 * won't register the auth protocol type id's properly */

	/* IMAP and POP need at least a password,
	 * which camel uses if we specify NULL.
	 * Camel use a password for IMAP or POP if we specify NULL,
	 * For IMAP, at least it will report an error if we use "Password", "Login" or "Plain".
	 * (POP is know to report an error for Login too. Probably Password and Plain too.) */
	protocol = im_protocol_registry_get_protocol_by_type (self, IM_PROTOCOLS_STORE_IMAP);
	im_account_protocol_set_custom_secure_auth_mech (IM_ACCOUNT_PROTOCOL (protocol), IM_PROTOCOLS_AUTH_NONE, NULL);
	im_account_protocol_set_custom_secure_auth_mech (IM_ACCOUNT_PROTOCOL (protocol), IM_PROTOCOLS_AUTH_PASSWORD, NULL);
	protocol = im_protocol_registry_get_protocol_by_type (self, IM_PROTOCOLS_STORE_POP);
	im_account_protocol_set_custom_secure_auth_mech (IM_ACCOUNT_PROTOCOL (protocol), IM_PROTOCOLS_AUTH_NONE, NULL);
	im_account_protocol_set_custom_secure_auth_mech (IM_ACCOUNT_PROTOCOL (protocol), IM_PROTOCOLS_AUTH_PASSWORD, NULL);
}

ImProtocolType
im_protocol_registry_get_imap_type_id (void)
{
	return imap_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_pop_type_id (void)
{
	return pop_protocol_type_id;
}

ImProtocolType
im_protocol_registry_get_maildir_type_id (void)
{
	return maildir_protocol_type_id;
}

ImProtocolType
im_protocol_registry_get_mbox_type_id (void)
{
	return mbox_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_smtp_type_id (void)
{
	return smtp_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_sendmail_type_id (void)
{
	return sendmail_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_none_connection_type_id (void)
{
	return none_connection_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_ssl_connection_type_id (void)
{
	return ssl_connection_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_tls_connection_type_id (void)
{
	return tls_connection_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_tlsop_connection_type_id (void)
{
	return tlsop_connection_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_none_auth_type_id (void)
{
	return none_auth_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_password_auth_type_id (void)
{
	return password_auth_protocol_type_id;
}

ImProtocolType 
im_protocol_registry_get_crammd5_auth_type_id (void)
{
	return crammd5_auth_protocol_type_id;
}

