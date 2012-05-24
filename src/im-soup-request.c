/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-soup-request.c : SoupRequest implementing iwk: protocol, invoked from JS code */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "im-soup-request.h"

#include "im-account-mgr.h"
#include "im-account-mgr-helpers.h"
#include "im-account-protocol.h"
#include "im-account-settings.h"
#include "im-content-id-request.h"
#include "im-error.h"
#include "im-mail-ops.h"
#include "im-protocol-registry.h"
#include "im-server-account-settings.h"
#include "im-service-mgr.h"

#include <camel/camel.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <libsoup/soup-uri.h>

#define IM_X_MAILER ("Igalia WebKit Mail " VERSION)

G_DEFINE_TYPE (ImSoupRequest, im_soup_request, SOUP_TYPE_REQUEST)

struct _ImSoupRequestPrivate {
  gssize content_length;
};

static void
im_soup_request_init (ImSoupRequest *request)
{
  request->priv = G_TYPE_INSTANCE_GET_PRIVATE (request, IM_TYPE_SOUP_REQUEST, ImSoupRequestPrivate);
  request->priv->content_length = 0;
}

static void
im_soup_request_finalize (GObject *obj)
{
  G_OBJECT_CLASS (im_soup_request_parent_class)->finalize (obj);
}

static gboolean
im_soup_request_check_uri (SoupRequest  *request,
                              SoupURI      *uri,
                              GError      **error)
{
  return uri->host == NULL;
}

static ImServerAccountSettings *
create_store (GHashTable *params, GError **error)
{
	ImServerAccountSettings *settings = NULL;
	GError *_error = NULL;
	const char *protocol_str;
	const char *hostname;
	const char *port_str;
	const char *security_str;
	const char *username;

	ImProtocol *protocol = NULL;
	guint port;
	ImProtocol *security_protocol = NULL;

	protocol_str = (const char *) g_hash_table_lookup (params, "incoming-protocol-choice");
	hostname = (const char *) g_hash_table_lookup (params, "incoming-server-host");
	if (hostname == NULL || *hostname == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_HOST,
			     _("Missing incoming server host"));
		goto finish;
	}
	port_str = (const char *) g_hash_table_lookup (params, "incoming-server-port");
	security_str = (const char *) g_hash_table_lookup (params, "incoming-security-choice");
	username = (const char *) g_hash_table_lookup (params, "incoming-server-username");
	if (username == NULL || *username == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_USERNAME,
			     _("Missing incoming server username"));
		goto finish;
	}
	protocol = im_protocol_registry_get_protocol_by_name (im_protocol_registry_get_instance (),
							      IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
							      protocol_str);

	if (protocol == NULL || !IM_IS_ACCOUNT_PROTOCOL (protocol)) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_PROTOCOL,
			     _("Invalid incoming server protocol"));
		goto finish;
	}

	security_protocol = im_protocol_registry_get_protocol_by_name (im_protocol_registry_get_instance (),
								       IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
								       security_str);
	if (security_protocol == NULL) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_CONNECTION_PROTOCOL,
			     _("Invalid incoming server security protocol"));
		goto finish;
	}

	if (port_str == NULL || *port_str == '\0') {
		/* Default port for protocol */
		ImProtocolType security_protocol_type = im_protocol_get_type_id (security_protocol);

		if (im_protocol_registry_protocol_type_has_tag (im_protocol_registry_get_instance (),
								security_protocol_type,
								IM_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT)) {
			port = im_account_protocol_get_alternate_port (IM_ACCOUNT_PROTOCOL (protocol));
		} else {
			port = im_account_protocol_get_port (IM_ACCOUNT_PROTOCOL (protocol));
		}
	} else {
		port = g_ascii_strtoull (port_str, NULL, 10);
	}

	settings = im_server_account_settings_new ();
	im_server_account_settings_set_hostname (settings, hostname);
	im_server_account_settings_set_protocol (settings, im_protocol_get_type_id (protocol));
	im_server_account_settings_set_port (settings, port);
	im_server_account_settings_set_username (settings, username);
	im_server_account_settings_set_security_protocol (settings, im_protocol_get_type_id (security_protocol));

finish:
	if (_error) g_propagate_error (error, _error);

	return settings;
}

static ImServerAccountSettings *
create_transport (GHashTable *params, GError **error)
{
	ImServerAccountSettings *settings = NULL;
	GError *_error = NULL;
	const char *hostname;
	const char *port_str;
	const char *security_str;
	const char *auth_str;
	const char *username;

	ImProtocol *protocol = NULL;
	guint port;
	ImProtocol *security_protocol = NULL;
	ImProtocol *auth_protocol = NULL;

	hostname = (const char *) g_hash_table_lookup (params, "outgoing-server-host");
	if (hostname == NULL || *hostname == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_HOST,
			     _("Missing outgoing server host"));
		goto finish;
	}
	port_str = (const char *) g_hash_table_lookup (params, "outgoing-server-port");
	security_str = (const char *) g_hash_table_lookup (params, "outgoing-security-choice");
	username = (const char *) g_hash_table_lookup (params, "outgoing-server-username");
	if (username == NULL || *username == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_USERNAME,
			     _("Missing outgoing server username"));
		goto finish;
	}
	auth_str = (const char *) g_hash_table_lookup (params, "outgoing-auth-choice");

	protocol = im_protocol_registry_get_protocol_by_name (im_protocol_registry_get_instance (),
							      IM_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
							      "smtp");

	security_protocol = im_protocol_registry_get_protocol_by_name (im_protocol_registry_get_instance (),
								       IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
								       security_str);
	if (security_protocol == NULL) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_CONNECTION_PROTOCOL,
			     _("Invalid outgoing server security protocol"));
		goto finish;
	}

	auth_protocol = im_protocol_registry_get_protocol_by_name (im_protocol_registry_get_instance (),
								   IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
								   auth_str);
	if (auth_protocol == NULL) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_AUTH_PROTOCOL,
			     _("Invalid outgoing server authentication protocol"));
		goto finish;
	}

	if (port_str == NULL || *port_str == '\0') {
		/* Default port for protocol */
		ImProtocolType security_protocol_type = im_protocol_get_type_id (security_protocol);

		if (im_protocol_registry_protocol_type_has_tag (im_protocol_registry_get_instance (),
								security_protocol_type,
								IM_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT)) {
			port = im_account_protocol_get_alternate_port (IM_ACCOUNT_PROTOCOL (protocol));
		} else {
			port = im_account_protocol_get_port (IM_ACCOUNT_PROTOCOL (protocol));
		}
	} else {
		port = g_ascii_strtoull (port_str, NULL, 10);
	}

	settings = im_server_account_settings_new ();
	im_server_account_settings_set_hostname (settings, hostname);
	im_server_account_settings_set_protocol (settings, im_protocol_get_type_id (protocol));
	im_server_account_settings_set_port (settings, port);
	im_server_account_settings_set_username (settings, username);
	im_server_account_settings_set_security_protocol (settings, im_protocol_get_type_id (security_protocol));
	im_server_account_settings_set_auth_protocol (settings, im_protocol_get_type_id (auth_protocol));

finish:
	if (_error) g_propagate_error (error, _error);

	return settings;
}

static ImAccountSettings*
create_account (GHashTable *params, ImServerAccountSettings *store, ImServerAccountSettings *transport, GError **error)
{
	ImAccountSettings *settings = NULL;
	GError *_error = NULL;
	const char *fullname;
	const char *accountname;
	const char *emailaddress;

	fullname = g_hash_table_lookup (params, (char *) "fullname");
	accountname = g_hash_table_lookup (params, (char *) "accountname");
	emailaddress = g_hash_table_lookup (params, (char *) "emailaddress");

	if (emailaddress == NULL || *emailaddress == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_EMAIL_ADDRESS,
			     _("Invalid email address"));
		goto finish;
	}
	if (accountname == NULL || *accountname == '\0') {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SETTINGS_INVALID_ACCOUNT_NAME,
			     _("Missing account name"));
		goto finish;
	}

	settings = im_account_settings_new ();
	im_account_settings_set_email_address (settings, emailaddress);
	im_account_settings_set_fullname (settings, fullname);
	im_account_settings_set_display_name (settings, accountname);
	im_account_settings_set_enabled (settings, TRUE);
	im_account_settings_set_store_settings (settings, store);
	im_account_settings_set_transport_settings (settings, transport);
finish:
	if (_error) g_propagate_error (error, _error);

	return settings;
}

static void
response_finish (GAsyncResult *result, const gchar *callback_id, JsonNode *result_node, GError *error)
{
	ImSoupRequest *request;
	JsonGenerator *generator;
	char *json_data;
	gssize length;
	GString *buffer = g_string_new (NULL);
	GInputStream *input_stream;
	JsonBuilder *builder;

	builder = json_builder_new ();

	request = (ImSoupRequest *) g_async_result_get_source_object (result);
	
	if (callback_id)
		g_string_append_printf (buffer, "%s (", callback_id);

	json_builder_begin_object (builder);

	json_builder_set_member_name (builder, "is_ok");
	json_builder_add_boolean_value (builder, error == NULL);
	if (error) {
		json_builder_set_member_name (builder, "error");
		json_builder_add_string_value (builder, error->message);
	}
	if (result_node) {
		json_builder_set_member_name (builder, "result");
		json_builder_add_value (builder, json_node_copy (result_node));
	}
	json_builder_end_object (builder);

	generator = json_generator_new ();
	json_generator_set_root (generator, json_builder_get_root (builder));
	json_data = json_generator_to_data (generator, NULL);
	g_object_unref (generator);

	g_string_append (buffer, json_data);
	g_free (json_data);

	if (callback_id)
		g_string_append (buffer, ")");

	length = buffer->len;

	input_stream = g_memory_input_stream_new_from_data (g_string_free (buffer, FALSE),
							    length,
							    g_free);
	g_simple_async_result_set_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result),
						   input_stream,
						   g_object_unref);
	request->priv->content_length = length;

	g_simple_async_result_complete_in_idle (G_SIMPLE_ASYNC_RESULT (result));
	g_object_unref (request);
}

static void
add_account (GAsyncResult *result,
	     GHashTable *request_params)
{
	GError *_error = NULL;
	ImAccountSettings *account = NULL;
	ImServerAccountSettings *store = NULL, *transport = NULL;
	GHashTable *params;
	const char *form_data;
	const char *callback_id;

	callback_id = g_hash_table_lookup (request_params, "callback");
	form_data = g_hash_table_lookup (request_params, "formData");
	params = soup_form_decode (form_data);

	store = create_store (params, &_error);
	if (store == NULL) {
		goto finish;
	}

	transport = create_transport (params, &_error);
	if (transport == NULL) {
		goto finish;
	}

	account = create_account (params, store, transport, &_error);
	if (account == NULL) {
		goto finish;
	}

	if (!im_account_mgr_add_account_from_settings (im_account_mgr_get_instance (),
						       account)) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_ACCOUNT_MGR_ADD_ACCOUNT_FAILED,
			     _("Failed to create account"));
	}

finish:
	response_finish (result, callback_id, NULL, _error);

	if (params) g_hash_table_unref (params);
	if (store) g_object_unref (store);
	if (transport) g_object_unref (transport);
	if (account) g_object_unref (account);
	if (_error) g_error_free (_error);
}

static void
get_accounts (GAsyncResult *result,
	      GHashTable *request_params)
{
	  GSList *account_ids, *node;
	  ImAccountMgr *mgr = im_account_mgr_get_instance ();
	  char *default_account;
	  const char *callback_id;
	  JsonBuilder *builder;
	  JsonNode *result_node;

	  callback_id = g_hash_table_lookup (request_params, "callback");

	  account_ids = im_account_mgr_get_account_ids (mgr, TRUE);
	  default_account = im_account_mgr_get_default_account (mgr);

	  builder = json_builder_new ();
	  json_builder_begin_array (builder);
	  for (node = account_ids; node != NULL; node = g_slist_next (node)) {
		  char *id = (char *) node->data;
		  ImAccountSettings *settings;
		  JsonNode *account_json;

		  settings = im_account_mgr_load_account_settings (mgr, id);

		  account_json = json_gobject_serialize (G_OBJECT (settings));
		  json_builder_add_value (builder, account_json);
	  }
	  json_builder_end_array (builder);

	  g_free (default_account);
	  im_account_mgr_free_account_ids (account_ids);

	  result_node = json_builder_get_root (builder);	  
	  g_object_unref (builder);
	  response_finish (result, callback_id, result_node, NULL);
	  json_node_free (result_node);
}

typedef struct _SyncFoldersData {
	GAsyncResult *result;
	gchar *callback_id;
	GCancellable *cancellable;
	GError *error;
	gint count;
	GHashTable *folder_infos;
	GHashTable *stores;
	GHashTable *non_storage;
} SyncFoldersData;

static void
dump_local_folder (JsonBuilder *builder, CamelFolder *folder)
{
	json_builder_set_member_name (builder, "unreadCount");
	json_builder_add_int_value (builder,
				    camel_folder_get_unread_message_count (folder));
	json_builder_set_member_name (builder, "messageCount");
	json_builder_add_int_value (builder,
				    camel_folder_get_message_count (folder));
	json_builder_set_member_name (builder, "noSelect");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "favourite");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isInbox");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isSent");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isTrash");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isLocal");
	json_builder_add_boolean_value (builder, TRUE);
}

static void
dump_drafts_folder (JsonBuilder *builder, CamelFolder *folder)
{
	json_builder_set_member_name (builder, IM_LOCAL_DRAFTS_TAG);
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "fullName");
	json_builder_add_string_value (builder, IM_LOCAL_DRAFTS_TAG);
	json_builder_set_member_name (builder, "displayName");
	json_builder_add_string_value (builder, camel_folder_get_display_name (folder));
	json_builder_set_member_name (builder, "isOutbox");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isDrafts");
	json_builder_add_boolean_value (builder, TRUE);
	dump_local_folder (builder, folder);
	json_builder_end_object (builder);
}

static void
dump_outbox_folder (JsonBuilder *builder, CamelFolder *folder)
{
	json_builder_set_member_name (builder, IM_LOCAL_OUTBOX_TAG);
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "fullName");
	json_builder_add_string_value (builder, IM_LOCAL_OUTBOX_TAG);
	json_builder_set_member_name (builder, "displayName");
	json_builder_add_string_value (builder, _("Outbox"));
	json_builder_set_member_name (builder, "isOutbox");
	json_builder_add_boolean_value (builder, TRUE);
	json_builder_set_member_name (builder, "Drafts");
	json_builder_add_boolean_value (builder, FALSE);
	dump_local_folder (builder, folder);
	json_builder_end_object (builder);
}

static void
dump_folder_info (JsonBuilder *builder,
		  CamelFolderInfo *fi,
		  CamelFolder *drafts,
		  CamelFolder *outbox,
		  gboolean non_storage)
{
	gboolean dumpedLocalFolders = FALSE;
	while (fi) {
		/* If no account updated folders, then it's a non storage account,
		 * we retrieve the folder info as inbox */
		if (!non_storage) {
			json_builder_set_member_name (builder, fi->full_name);
			json_builder_begin_object (builder);
			json_builder_set_member_name (builder, "fullName");
			json_builder_add_string_value (builder, fi->full_name);
			json_builder_set_member_name (builder, "displayName");
			json_builder_add_string_value (builder, fi->display_name);
			json_builder_set_member_name (builder, "favourite");
			json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_CHECK_FOR_NEW);
			json_builder_set_member_name (builder, "isInbox");
			json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_INBOX);
			json_builder_set_member_name (builder, "isLocal");
			json_builder_add_boolean_value (builder, FALSE);
			if (fi->parent) {
				json_builder_set_member_name (builder, "parentFullName");
				json_builder_add_string_value (builder, fi->parent->full_name);
			}
		} else {
			json_builder_set_member_name (builder, "INBOX");
			json_builder_begin_object (builder);
			json_builder_set_member_name (builder, "fullName");
			json_builder_add_string_value (builder, "INBOX");
			json_builder_set_member_name (builder, "displayName");
			json_builder_add_string_value (builder, _("Inbox"));
			json_builder_set_member_name (builder, "favourite");
			json_builder_add_boolean_value (builder, TRUE);
			json_builder_set_member_name (builder, "isInbox");
			json_builder_add_boolean_value (builder, TRUE);
			json_builder_set_member_name (builder, "isLocal");
			json_builder_add_boolean_value (builder, TRUE);
		}
		json_builder_set_member_name (builder, "unreadCount");
		json_builder_add_int_value (builder, fi->unread);
		json_builder_set_member_name (builder, "messageCount");
		json_builder_add_int_value (builder, fi->total);
		json_builder_set_member_name (builder, "noSelect");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_NOSELECT);
		json_builder_set_member_name (builder, "isOutbox");
		json_builder_add_boolean_value (builder, FALSE);
		json_builder_set_member_name (builder, "isSent");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_SENT);
		json_builder_set_member_name (builder, "isTrash");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_TRASH);
		json_builder_set_member_name (builder, "isDrafts");
		json_builder_add_boolean_value (builder, FALSE);
		json_builder_end_object (builder);

		if (!dumpedLocalFolders) {
			if (drafts)
				dump_drafts_folder (builder, drafts);
			if (outbox)
				dump_outbox_folder (builder, outbox);
			dumpedLocalFolders = TRUE;
		}

		if (fi->child) {
			dump_folder_info (builder, fi->child, NULL, NULL, non_storage);
		}
		fi = fi->next;
	}
}

static void
free_service_store_folder_info (const gchar *account_id,
				CamelStore *service_store,
				CamelFolderInfo *fi)
{
	ImServiceMgr *service_mgr;
	CamelStore *store;

	service_mgr = im_service_mgr_get_instance ();

	if (im_service_mgr_has_local_inbox (service_mgr, account_id))
		store = im_service_mgr_get_local_store (service_mgr);
	else
		store = service_store;
	camel_store_free_folder_info (store, fi);
}

typedef struct _SyncAccountData {
	GAsyncResult *result;
	gchar *callback_id;
} SyncAccountData;

static void
finish_sync_account (SyncAccountData *data,
		     const gchar *account_id,
		     CamelFolderInfo *fi,
		     GError *error)
{
	ImServiceMgr *service_mgr;
	CamelFolder *drafts;
	CamelFolder *outbox;
	JsonBuilder *builder;
	JsonNode *result_node;
	char *display_name;

	service_mgr = im_service_mgr_get_instance ();

	drafts = im_service_mgr_get_drafts (service_mgr, NULL, NULL);
	outbox = im_service_mgr_get_outbox (service_mgr, account_id, NULL, NULL);

	builder = json_builder_new ();
	json_builder_begin_object (builder);

	json_builder_set_member_name (builder, "accountId");
	json_builder_add_string_value (builder, account_id);

	json_builder_set_member_name (builder, "accountName");
	display_name = im_account_mgr_get_display_name (im_account_mgr_get_instance (),
							account_id);
	json_builder_add_string_value (builder, display_name);
	g_free (display_name);

	json_builder_set_member_name (builder, "folders");
	json_builder_begin_object (builder);
	if (fi) {
		gboolean non_storage;

		non_storage = im_service_mgr_has_local_inbox (service_mgr, account_id);
		dump_folder_info (builder, fi, drafts, outbox, non_storage);
	}
	json_builder_end_object (builder); /* folders */
	json_builder_end_object (builder); /* top */

	if (outbox) g_object_unref (outbox);
	if (drafts) g_object_unref (drafts);

	result_node = json_builder_get_root (builder);
	g_object_unref (builder);

	response_finish (data->result,
			 data->callback_id,
			 result_node,
			 error);
	json_node_free (result_node);

	g_object_unref (data->result);
	g_free (data->callback_id);
	g_free (data);
}

static void
sync_account_synchronize_store_cb (GObject *source_object,
				   GAsyncResult *res,
				   gpointer userdata)
{
	CamelStore *service_store = (CamelStore *) source_object;
	SyncAccountData *data = (SyncAccountData *) userdata;
	CamelFolderInfo *fi;
	GError *error = NULL;
	gchar *account_id;

	account_id = im_account_mgr_get_server_parent_account_name 
		(im_account_mgr_get_instance (),
		 camel_service_get_uid (CAMEL_SERVICE (service_store)),
		 IM_ACCOUNT_TYPE_STORE);
	fi = im_mail_op_synchronize_store_finish (CAMEL_STORE (service_store),
						  res, &error);
	finish_sync_account (data, account_id, fi, error);
	if (fi) {
		free_service_store_folder_info (account_id,
						service_store,
						fi);
	}
	g_free (account_id);
	if (error) g_error_free (error);
}

static void
sync_account (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	  SyncAccountData *data = g_new0(SyncAccountData, 1);
	  const gchar *account_id;
	  CamelStore *store;

	  data->result = g_object_ref (result);
	  data->callback_id = g_strdup ((char *) g_hash_table_lookup (params, "callback"));

	  account_id = g_hash_table_lookup (params, "account");
	  store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
							     account_id,
							     IM_ACCOUNT_TYPE_STORE);

	  if (CAMEL_IS_STORE (store)) {
		  im_mail_op_synchronize_store_async (store,
						      G_PRIORITY_DEFAULT_IDLE,
						      cancellable,
						      sync_account_synchronize_store_cb,
						      data);
	  } else {
		  GError *_error = NULL;
		  g_set_error (&_error, IM_ERROR_DOMAIN,
			       IM_ERROR_INTERNAL,
			       _("Account does not exist"));
		  finish_sync_account (data, NULL, NULL, _error);
		  g_error_free (_error);
	  }
}

typedef struct _RunSendQueueData {
	GAsyncResult *result;
	gchar *callback_id;
} RunSendQueueData;

static void
finish_run_send_queue (RunSendQueueData *data, GError *error)
{
	response_finish (data->result, data->callback_id, NULL, error);
	g_object_unref (data->result);
	g_free (data->callback_id);
	g_free (data);
}

static void
run_send_queue_mail_op_cb (GObject *source_object,
			   GAsyncResult *result,
			   gpointer userdata)
{
	RunSendQueueData *data = userdata;
	CamelFolder *outbox = (CamelFolder *) source_object;
	GError *_error = NULL;

	im_mail_op_run_send_queue_finish (outbox, result, &_error);
	finish_run_send_queue (data, _error);
	if (_error) g_error_free (_error);
}

static void
run_send_queue (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	GError *_error = NULL;
	RunSendQueueData *data = g_new0(RunSendQueueData, 1);
	const gchar *account_id = g_hash_table_lookup (params, "account");
	CamelFolder *outbox;

	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	outbox = im_service_mgr_get_outbox (im_service_mgr_get_instance (),
					    account_id,
					    cancellable,
					    &_error);
	if (outbox) {
		im_mail_op_run_send_queue_async (outbox,
						 G_PRIORITY_DEFAULT_IDLE,
						 cancellable,
						 run_send_queue_mail_op_cb,
						 data);
		g_object_unref (outbox);
	} else {
		if (_error == NULL)
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_INTERNAL,
				     _("Could not find account %s outbox"), account_id);
		finish_run_send_queue (data, _error);
		g_error_free (_error);
	}
}

typedef struct _SyncOutboxStoreData {
	GAsyncResult *result;
	gchar *callback_id;
} SyncOutboxStoreData;

static void
finish_sync_outbox_store (SyncOutboxStoreData *data, GError *error)
{
	response_finish (data->result, data->callback_id, NULL, error);
	g_object_unref (data->result);
	g_free (data->callback_id);
	g_free (data);
}

static void
sync_outbox_store_synchronize_cb (GObject *source_object,
				  GAsyncResult *result,
				  gpointer userdata)
{
	SyncOutboxStoreData *data = userdata;
	CamelStore *outbox_store = (CamelStore *) source_object;
	GError *_error = NULL;
	CamelFolderInfo *fi;

	fi = im_mail_op_synchronize_store_finish (outbox_store, result, &_error);
	if (fi) camel_store_free_folder_info (outbox_store, fi);
	finish_sync_outbox_store (data, _error);
	if (_error) g_error_free (_error);
}

static void
sync_outbox_store (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	GError *_error = NULL;
	SyncOutboxStoreData *data = g_new0(SyncOutboxStoreData, 1);
	CamelStore *outbox_store;

	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	outbox_store = im_service_mgr_get_outbox_store (im_service_mgr_get_instance ());
	if (outbox_store) {
		im_mail_op_synchronize_store_async (outbox_store,
						    G_PRIORITY_DEFAULT_IDLE,
						    cancellable,
						    sync_outbox_store_synchronize_cb,
						    data);
	} else {
		if (_error == NULL)
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_INTERNAL,
				     _("Could not find outbox store"));
		finish_sync_outbox_store (data, _error);
		g_error_free (_error);
	}
}

typedef struct _FetchMessagesData {
	GAsyncResult *result;
	gchar *callback_id;
	gchar *newest_uid;
	gchar *oldest_uid;
	gint count;
} FetchMessagesData;

static void
finish_fetch_messages (FetchMessagesData *data, JsonNode *result_node, GError *error)
{
	response_finish (data->result, data->callback_id, result_node, error);
	g_object_unref (data->result);
	g_free (data->callback_id);
	g_free (data->newest_uid);
	g_free (data->oldest_uid);
	g_free (data);
}

static void
dump_message_info (JsonBuilder *builder,
		   CamelMessageInfo *mi)
{
	CamelMessageFlags flags;

	flags = camel_message_info_flags (mi);
	json_builder_begin_object(builder);
	json_builder_set_member_name (builder, "uid");
	json_builder_add_string_value (builder,
				       camel_message_info_uid (mi));

	json_builder_set_member_name (builder, "from");
	json_builder_add_string_value (builder,
				       camel_message_info_from (mi));
	json_builder_set_member_name (builder, "to");
	json_builder_add_string_value (builder,
				       camel_message_info_to (mi));
	json_builder_set_member_name (builder, "cc");
	json_builder_add_string_value (builder,
				       camel_message_info_cc (mi));
	json_builder_set_member_name (builder, "mlist");
	json_builder_add_string_value (builder,
				       camel_message_info_mlist (mi));

	json_builder_set_member_name (builder, "dateReceived");
	json_builder_add_int_value (builder,
				    camel_message_info_date_received (mi));
	json_builder_set_member_name (builder, "dateSent");
	json_builder_add_int_value (builder,
				    camel_message_info_date_sent (mi));

	json_builder_set_member_name (builder, "subject");
	json_builder_add_string_value (builder,
				       camel_message_info_subject (mi));
	json_builder_set_member_name (builder, "size");
	json_builder_add_int_value (builder,
				       camel_message_info_size (mi));

	json_builder_set_member_name (builder, "unread");
	json_builder_add_boolean_value (builder,
					!(flags & CAMEL_MESSAGE_SEEN));
	json_builder_set_member_name (builder, "deleted");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_DELETED);
	json_builder_set_member_name (builder, "draft");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_DRAFT);
	json_builder_set_member_name (builder, "hasAttachments");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_ATTACHMENTS);
	json_builder_set_member_name (builder, "unblockImages");
	json_builder_add_boolean_value (builder,
					camel_message_info_user_flag (mi,
								      "unblockImages"));
	json_builder_end_object(builder);
}

static void
dump_mailing_list_as_string (JsonBuilder *builder,
			     CamelMimeMessage *message)
{
	gchar *mlist;
	mlist = camel_header_raw_check_mailing_list (&(((CamelMimePart *) message)->headers));
	json_builder_add_string_value (builder, mlist);
	g_free (mlist);
}

static void
fetch_messages_refresh_info_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	FetchMessagesData *data = (FetchMessagesData *) userdata;
	GError *error = NULL;
	CamelFolder *folder = NULL;
	GPtrArray *uids;
	int i, j;
	JsonBuilder *builder;
	JsonNode *result_node;

	builder = json_builder_new ();
	json_builder_begin_object (builder);

	im_mail_op_refresh_folder_info_finish (IM_SERVICE_MGR (source_object),
					       result,
					       &folder,
					       &error);

	if (folder) {

		uids = camel_folder_get_uids (folder);
		camel_folder_sort_uids (folder, uids);
		
		/* Fetch first new messages */
		json_builder_set_member_name (builder, "newMessages");
		json_builder_begin_array(builder);
		i = uids->len - 1;
		if (data->newest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				CamelMessageInfo *mi;
				if (g_strcmp0 (uid, data->newest_uid) == 0)
					break;
				mi = camel_folder_get_message_info (folder, uid);
				dump_message_info (builder, mi);
				camel_folder_free_message_info (folder, mi);
				i--;
			}
		}
		json_builder_end_array (builder);
		if (data->oldest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				if (g_strcmp0 (uid, data->oldest_uid) == 0)
					break;
				i--;
			}
			i--;
		}
		json_builder_set_member_name (builder, "messages");
		json_builder_begin_array (builder);
		for (j = 0; j < data->count; j++) {
			const char *uid;
			CamelMessageInfo *mi;
			if (i < 0)
				break;
			uid = (const char *) uids->pdata[i];
			mi = camel_folder_get_message_info (folder, uid);
			dump_message_info (builder, mi);
			camel_folder_free_message_info (folder, mi);
			i--;
		}
		json_builder_end_array(builder);
		camel_folder_free_uids (folder, uids);
	}
	json_builder_end_object (builder);
		
	result_node = json_builder_get_root (builder);
	g_object_unref (builder);

	/* Unless we got a cancel, we ignore the error */
	if (error && 
	    !(error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)) {
		g_clear_error (&error);
	}

	finish_fetch_messages (data, result_node, error);

	if (error) g_error_free (error);
	json_node_free (result_node);
}

static void
fetch_messages (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	const char *account_id;
	const char *folder_name;
	FetchMessagesData *data = g_new0 (FetchMessagesData, 1);
	const gchar *newest_uid;
	const gchar *oldest_uid;
	const gchar *count_str;

	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	newest_uid = g_hash_table_lookup (params, "newestUid");
	if (newest_uid == '\0' || g_strcmp0 (newest_uid, "null") == 0) newest_uid = NULL;
	data->newest_uid = g_strdup (newest_uid);
	oldest_uid = g_hash_table_lookup (params, "oldestUid");
	if (oldest_uid == '\0' || g_strcmp0 (oldest_uid, "null") == 0) oldest_uid = NULL;
	data->oldest_uid = g_strdup (oldest_uid);
	count_str = g_hash_table_lookup (params, "count");
	if (count_str) {
		data->count = atoi(count_str);
	}
	if (data->count < 0)
		data->count = 20;


	account_id = g_hash_table_lookup (params, "account");
	folder_name = g_hash_table_lookup (params, "folder");

	im_mail_op_refresh_folder_info_async (im_service_mgr_get_instance (),
					      account_id,
					      folder_name,
					      G_PRIORITY_DEFAULT_IDLE,
					      cancellable,
					      fetch_messages_refresh_info_cb,
					      data);
}

typedef struct _GetMessageData {
	GAsyncResult *result;
	gchar *callback_id;
	CamelURL *url;
} GetMessageData;

static void
finish_get_message (GetMessageData *data, JsonNode *result_node, GError *error)
{
	response_finish (data->result, data->callback_id, result_node, error);
	
	g_object_unref (data->result);
	g_free (data->callback_id);
	if (data->url) camel_url_free (data->url);
	g_free (data);
}

/* forward declaration */
static void dump_data_wrapper (JsonBuilder *builder,
			       CamelURL *url,
			       CamelDataWrapper *wrapper);

static void
dump_internet_address (JsonBuilder *builder,
		       CamelInternetAddress *address)
{
	json_builder_begin_array (builder);

	if (address) {
		gint len, i;

		len = camel_address_length (CAMEL_ADDRESS (address));

		for (i = 0; i < len; i++) {
			const gchar *name, *email;
			if (camel_internet_address_get (address, i, &name, &email)) {
				json_builder_begin_object (builder);
				json_builder_set_member_name (builder, "displayName");
				json_builder_add_string_value (builder, name);
				json_builder_set_member_name (builder, "emailAddress");
				json_builder_add_string_value (builder, email);
				json_builder_end_object (builder);
			}
		}
	}
	json_builder_end_array (builder);
}

static void
dump_message (JsonBuilder *builder,
	      CamelMimeMessage *message)
{
	json_builder_set_member_name (builder, "subject");
	json_builder_add_string_value (builder, camel_mime_message_get_subject (message));

	json_builder_set_member_name (builder, "from");
	dump_internet_address (builder, camel_mime_message_get_from (message));
	json_builder_set_member_name (builder, "to");
	dump_internet_address (builder, camel_mime_message_get_recipients (message, "To"));
	json_builder_set_member_name (builder, "cc");
	dump_internet_address (builder, camel_mime_message_get_recipients (message, "Cc"));
	json_builder_set_member_name (builder, "bcc");
	dump_internet_address (builder, camel_mime_message_get_recipients (message, "Bcc"));
	json_builder_set_member_name (builder, "replyTo");
	dump_internet_address (builder, camel_mime_message_get_reply_to (message));
	json_builder_set_member_name (builder, "mlist");
	dump_mailing_list_as_string (builder, message);
}

static void
dump_header_params (JsonBuilder *builder,
		    struct _camel_header_param *params)
{
	struct _camel_header_param *param;
	json_builder_begin_object (builder);
	param = params;
	while (param != NULL) {
		json_builder_set_member_name (builder, param->name);
		json_builder_add_string_value (builder, param->value);
		param = param->next;
	}
	json_builder_end_object (builder);
}

static void
dump_content_disposition (JsonBuilder *builder,
			  const CamelContentDisposition *disposition)
{
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "disposition");
	json_builder_add_string_value (builder, disposition?disposition->disposition:"");
	json_builder_set_member_name (builder, "params");
	dump_header_params (builder, disposition?disposition->params:NULL);
	json_builder_end_object (builder);
}

static void
dump_content_type (JsonBuilder *builder,
		   CamelContentType *content_type)
{
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "type");
	json_builder_add_string_value (builder, content_type->type);
	json_builder_set_member_name (builder, "subType");
	json_builder_add_string_value (builder, content_type->subtype);
	json_builder_set_member_name (builder, "params");
	dump_header_params (builder, content_type->params);
	json_builder_end_object (builder);
}

static void
dump_part (JsonBuilder *builder,
	   CamelMimePart *part)
{
	json_builder_set_member_name (builder, "disposition");
	json_builder_add_string_value (builder, camel_mime_part_get_disposition (part));
	json_builder_set_member_name (builder, "description");
	json_builder_add_string_value (builder, camel_mime_part_get_description (part));
	json_builder_set_member_name (builder, "filename");
	json_builder_add_string_value (builder, camel_mime_part_get_filename (part));
	json_builder_set_member_name (builder, "contentId");
	json_builder_add_string_value (builder, camel_mime_part_get_content_id (part));
	json_builder_set_member_name (builder, "encoding");
	json_builder_add_string_value (builder,
				       camel_transfer_encoding_to_string (camel_mime_part_get_encoding (part)));
	json_builder_set_member_name (builder, "contentType");
	dump_content_type (builder, camel_mime_part_get_content_type (part));
	json_builder_set_member_name (builder, "contentDisposition");
	dump_content_disposition (builder, camel_mime_part_get_content_disposition (part));
	json_builder_set_member_name (builder, "contentLocation");
	json_builder_add_string_value (builder, camel_mime_part_get_content_location (part));
	json_builder_set_member_name (builder, "isMessage");
	json_builder_add_boolean_value (builder, CAMEL_IS_MIME_MESSAGE (part));

	if (CAMEL_IS_MIME_MESSAGE (part)) {
		dump_message (builder, CAMEL_MIME_MESSAGE (part));
	}
}

static void
dump_multipart (JsonBuilder *builder,
		CamelURL *url,
		CamelMultipart *multipart)
{
	gint i, count;
	json_builder_set_member_name (builder, "parts");
	count = camel_multipart_get_number (multipart);
	json_builder_begin_array (builder);
	for (i = 0; i < count; i++) {
		CamelMimePart *part;
		CamelURL *sub_url;
		gchar *sub_path;

		part = camel_multipart_get_part (multipart, i);
		if (!url->path || url->path[0] == '\0' ||g_strcmp0 (url->path, "/") == 0)
			sub_path = g_strdup_printf ("/%d", i);
		else
			sub_path = g_strdup_printf ("%s/%d", url->path, i);
		sub_url = camel_url_copy (url);
		camel_url_set_path (sub_url, sub_path);
		g_free (sub_path);
		dump_data_wrapper (builder, sub_url, CAMEL_DATA_WRAPPER (part));
		camel_url_free (sub_url);
	}
	json_builder_end_array (builder);
}

static void
dump_medium (JsonBuilder *builder,
	     CamelURL *url,
	     CamelMedium *medium)
{
	GArray *headers;
	gint i;

	json_builder_set_member_name (builder, "mediumHeaders");
	json_builder_begin_array (builder);
	headers = camel_medium_get_headers (medium);
	for (i = 0; i < headers->len; i++) {
		CamelMediumHeader header;
		header = g_array_index (headers, CamelMediumHeader, i);

		json_builder_begin_object (builder);
		json_builder_set_member_name (builder, "name");
		json_builder_add_string_value (builder, header.name);
		json_builder_set_member_name (builder, "value");
		json_builder_add_string_value (builder, header.value);
		json_builder_end_object (builder);
	}
	camel_medium_free_headers (medium, headers);
	json_builder_end_array (builder);

	json_builder_set_member_name (builder, "isMimePart");
	json_builder_add_boolean_value (builder, CAMEL_IS_MIME_PART (medium));

	if (CAMEL_IS_MIME_PART (medium)) {
		dump_part (builder, CAMEL_MIME_PART (medium));
	}

	json_builder_set_member_name (builder, "content");
	dump_data_wrapper (builder, url, camel_medium_get_content (medium));
}

static void
dump_data_wrapper (JsonBuilder *builder,
		   CamelURL *url,
		   CamelDataWrapper *wrapper)
{
	char *uri;

	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "uri");
	uri = camel_url_to_string (url, 0);
	json_builder_add_string_value (builder, uri);
	g_free (uri);
	json_builder_set_member_name (builder, "mimeType");
	dump_content_type (builder, camel_data_wrapper_get_mime_type_field (wrapper));

	json_builder_set_member_name (builder, "isMultipart");
	json_builder_add_boolean_value (builder, CAMEL_IS_MULTIPART (wrapper));

	json_builder_set_member_name (builder, "isMedium");
	json_builder_add_boolean_value (builder, CAMEL_IS_MEDIUM (wrapper));

	if (CAMEL_IS_MULTIPART (wrapper)) {
		dump_multipart (builder, url, CAMEL_MULTIPART (wrapper));
	} else if (CAMEL_IS_MEDIUM (wrapper)) {
		dump_medium (builder, url, CAMEL_MEDIUM (wrapper));
	}
	
	json_builder_end_object (builder);
}

static CamelURL *
build_message_url (const char *account_name,
		   const char *folder_full_name,
		   const char *uid)
{
	CamelURL *url;
	gchar *hostname;

	url = camel_url_new ("cid:", NULL);
	hostname = im_content_id_request_build_hostname (account_name, folder_full_name, uid);
	camel_url_set_host (url, hostname);
	g_free (hostname);

	return url;
}

static void
get_message_mail_op_cb (GObject *source_object,
			GAsyncResult *result,
			gpointer userdata)
{
	GetMessageData *data = (GetMessageData *) userdata;
	GError *error = NULL;
	CamelMimeMessage *message;
	JsonNode *result_node = NULL;

	message = im_mail_op_get_message_finish (IM_SERVICE_MGR (source_object),
						 result, &error);

	if (error == NULL) {
		JsonBuilder *builder;

		builder = json_builder_new ();
		dump_data_wrapper (builder, data->url, CAMEL_DATA_WRAPPER (message));
		result_node = json_builder_get_root (builder);
		g_object_unref (builder);
	}

	finish_get_message (data, result_node, error);
	if (result_node) json_node_free (result_node);
	if (message) g_object_unref (message);
	if (error) g_error_free (error);
}

static void
get_message (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	const gchar *account_id;
	const gchar *folder_fullname;
	const gchar *message_uid;
	GetMessageData *data = g_new0 (GetMessageData, 1);

	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	account_id = g_strdup (g_hash_table_lookup (params, "account"));
	folder_fullname = g_strdup (g_hash_table_lookup (params, "folder"));
	message_uid = g_strdup (g_hash_table_lookup (params, "message"));
	data->url = build_message_url (account_id,
				       folder_fullname,
				       message_uid);

	im_mail_op_get_message_async (im_service_mgr_get_instance (),
				      account_id,
				      folder_fullname,
				      message_uid,
				      G_PRIORITY_DEFAULT_IDLE,
				      cancellable,
				      get_message_mail_op_cb,
				      data);
}

typedef struct _FlagMessageData {
	GAsyncResult *result;
	gchar *callback_id;
} FlagMessageData;

static void
flag_message_mail_op_cb (GObject *object,
			 GAsyncResult *result,
			 gpointer userdata)
{
	GError *_error = NULL;
	FlagMessageData *data = (FlagMessageData *) userdata;

	im_mail_op_flag_message_finish (IM_SERVICE_MGR (object),
					result, &_error);
	response_finish (data->result, data->callback_id, NULL, _error);
	if (_error) g_error_free (_error);
	g_object_unref (data->result);
	g_free (data->callback_id);
	g_free (data);
}

static void
flag_message (GAsyncResult *result, GHashTable *params, GCancellable *cancellable)
{
	const gchar *account_id;
	const gchar *folder_name;
	const gchar *message_uid;
	const gchar *set_flags, *unset_flags;
	FlagMessageData *data = g_new0 (FlagMessageData, 1);

	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	account_id = g_hash_table_lookup (params, "account");
	folder_name = g_hash_table_lookup (params, "folder");
	message_uid = g_hash_table_lookup (params, "message");
	set_flags = g_strdup (g_hash_table_lookup (params, "setFlags"));
	unset_flags = g_strdup (g_hash_table_lookup (params, "unsetFlags"));

	im_mail_op_flag_message_async (im_service_mgr_get_instance (),
				       account_id,
				       folder_name,
				       message_uid,
				       set_flags,
				       unset_flags,
				       G_PRIORITY_DEFAULT_IDLE,
				       cancellable,
				       flag_message_mail_op_cb,
				       data);
}

typedef struct _ComposerSaveData {
	GAsyncResult *result;
	gchar *callback_id;
	GError *error;
	gchar *result_uid;
} ComposerSaveData;

static void
finish_composer_save (ComposerSaveData *data)
{
	JsonNode *result_node = NULL;
	if (data->result_uid) {
		JsonBuilder *builder;

		builder = json_builder_new ();
		json_builder_begin_object (builder);
		json_builder_set_member_name (builder, "messageUid");
		json_builder_add_string_value (builder, data->result_uid);
		json_builder_end_object (builder);

		result_node = json_builder_get_root (builder);
		g_object_unref (builder);
		g_free (data->result_uid);
	}
	response_finish (data->result, data->callback_id, result_node, data->error);
	if (result_node) json_node_free (result_node);

	g_object_unref (data->result);
	g_free (data->callback_id);
	if (data->error) g_error_free (data->error);
	g_free (data);
}

static void
composer_save_mail_op_cb (GObject *source_object,
			  GAsyncResult *result,
			  gpointer userdata)
{
	ComposerSaveData *data = (ComposerSaveData *) userdata;
	GError *_error = NULL;
	CamelFolder *folder = (CamelFolder *) source_object;

	im_mail_op_composer_save_finish (folder, result, &data->result_uid, &_error);

	if (_error)
		g_propagate_error (&data->error, _error);

	finish_composer_save (data);
}

static void
composer_save (GAsyncResult *result, GHashTable *params, gboolean is_sending, GCancellable *cancellable)
{
	GError *_error = NULL;
	const char *form_data;
	GHashTable *form_params;
	const char *account_id;
	const char *to, *cc, *bcc;
	const char *subject;
	const char *body;
	const char *attachments;
	CamelFolder *folder = NULL;
	CamelMimeMessage *message = NULL;
	ComposerSaveData *data = NULL;

	data = g_new0 (ComposerSaveData, 1);
	data->result = g_object_ref (result);
	data->callback_id = g_strdup (g_hash_table_lookup (params, "callback"));

	form_data = g_hash_table_lookup (params, "formData");
	form_params = soup_form_decode (form_data);

	account_id = g_hash_table_lookup (form_params, "composer-from-choice");
	to = g_hash_table_lookup (form_params, "composer-to");
	cc = g_hash_table_lookup (form_params, "composer-cc");
	bcc = g_hash_table_lookup (form_params, "composer-bcc");
	subject = g_hash_table_lookup (form_params, "composer-subject");
	body = g_hash_table_lookup (form_params, "composer-body");
	attachments = g_hash_table_lookup (form_params, "composer-attachments");

	if (_error == NULL && account_id == NULL) {
		g_set_error (&_error, IM_ERROR_DOMAIN,
			     IM_ERROR_SEND_INVALID_PARAMETERS,
			     _("No transport account specified trying to send the message"));
	}

	if (_error == NULL) {
		if (!im_account_mgr_account_exists (im_account_mgr_get_instance (),
						    account_id, FALSE)) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_SEND_INVALID_PARAMETERS,
				     _("Invalid transport account specified trying to send the message"));
		}
	}

	if (_error == NULL) {
		gchar *from_string;
		gint from_count;
		CamelInternetAddress *from_cia;

		message = camel_mime_message_new ();
		camel_medium_set_header (CAMEL_MEDIUM (message), "X-Mailer", IM_X_MAILER);
		camel_mime_message_set_subject (message, subject);
		camel_mime_message_set_date (message, CAMEL_MESSAGE_DATE_CURRENT, 0);

		from_string = im_account_mgr_get_from_string (im_account_mgr_get_instance (),
							      account_id, NULL);

		from_cia = camel_internet_address_new ();
		from_count = camel_address_unformat (CAMEL_ADDRESS (from_cia), from_string);
		if (from_count != 1) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_SEND_INVALID_ACCOUNT_FROM,
				     _("Account has an invalid from field"));
		} else {
			camel_mime_message_set_from (message,
						     from_cia);
		}
		g_object_unref (from_cia);
		g_free (from_string);
	}						


	if (_error == NULL) {
		CamelInternetAddress *to_cia, *cc_cia, *bcc_cia;
		gint to_count, cc_count, bcc_count;

		to_cia = camel_internet_address_new ();
		to_count = camel_address_unformat (CAMEL_ADDRESS (to_cia), to);
		cc_cia = camel_internet_address_new ();
		cc_count = camel_address_unformat (CAMEL_ADDRESS (cc_cia), cc);
		bcc_cia = camel_internet_address_new ();
		bcc_count = camel_address_unformat (CAMEL_ADDRESS (bcc_cia), bcc);

		if (to_count == -1 || cc_count == -1 || bcc_count == -1) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_SEND_PARSING_RECIPIENTS,
				     _("Failed to parse recipients"));
		} else if (is_sending && (to_count + cc_count + bcc_count == 0)) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_SEND_NO_RECIPIENTS,
				     _("User didn't set recipients trying to send"));
		} else {
			camel_mime_message_set_recipients (message,
							   CAMEL_RECIPIENT_TYPE_TO,
							   to_cia);
			camel_mime_message_set_recipients (message,
							   CAMEL_RECIPIENT_TYPE_CC,
							   cc_cia);
			camel_mime_message_set_recipients (message,
							   CAMEL_RECIPIENT_TYPE_BCC,
							   bcc_cia);
		}
		g_object_unref (to_cia);
		g_object_unref (cc_cia);
		g_object_unref (bcc_cia);
	}

	if (_error == NULL) {
		if (is_sending)
			folder = im_service_mgr_get_outbox (im_service_mgr_get_instance (),
							    account_id,
							    cancellable,
							    &_error);
		else
			folder = im_service_mgr_get_drafts (im_service_mgr_get_instance (),
							    cancellable,
							    &_error);
	}

	if (_error == NULL) {
		GList *uri_list = NULL;
		gchar **attachments_v, **node;

		attachments_v = g_strsplit (attachments, ",", 0);
		for (node = attachments_v; *node != NULL; node++) {
			uri_list = g_list_prepend (uri_list, g_strdup (*node));
		}
		g_strfreev (attachments_v);
		uri_list = g_list_reverse (uri_list);

		im_mail_op_composer_save_async (folder, message,
						body, uri_list,
						G_PRIORITY_DEFAULT_IDLE,
						cancellable,
						composer_save_mail_op_cb,
						data);
	}

	if (_error) {
		g_propagate_error (&data->error, _error);
		finish_composer_save (data);
	}

	if (message) g_object_unref (message);
	if (folder) g_object_unref (folder);
	g_hash_table_destroy (form_params);
}

static void
open_file_uri (GAsyncResult *result, GHashTable *params)
{
	const char *title, *attach_action, *callback_id;
	GtkWidget *dialog;
	JsonBuilder *builder;
	JsonNode *result_node;

	callback_id = g_hash_table_lookup (params, "callback");
	title = g_hash_table_lookup (params, "title");
	attach_action = g_hash_table_lookup (params, "attachAction");
	dialog = gtk_file_chooser_dialog_new (title, NULL,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      attach_action?attach_action:GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	builder = json_builder_new ();
	json_builder_begin_array (builder);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *uris, *node;
		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
		for (node = uris; node != NULL; node = g_slist_next (node)) {
			json_builder_add_string_value (builder, (char *) node->data);
			g_free (node->data);
		}
		g_slist_free (node);
	}
	json_builder_end_array (builder);
	gtk_widget_destroy (dialog);

	result_node = json_builder_get_root (builder);
	g_object_unref (builder);

	response_finish (result, callback_id, result_node, NULL);
	json_node_free (result_node);

}

static void
im_soup_request_send_async (SoupRequest          *soup_request,
			    GCancellable         *cancellable,
			    GAsyncReadyCallback   callback,
			    gpointer              userdata)
{
  ImSoupRequest *request = IM_SOUP_REQUEST (soup_request);
  SoupURI *uri = soup_request_get_uri (SOUP_REQUEST (request));
  GHashTable *params = NULL;
  GError *_error = NULL;
  GAsyncResult *result;

  if (soup_uri_get_query (uri)) {
	  params = soup_form_decode (soup_uri_get_query (uri));
  } else {
	  params = g_hash_table_new (g_str_hash, g_str_equal);
  }
  result = (GAsyncResult *) g_simple_async_result_new ((GObject *) request,
						       callback, userdata,
						       im_soup_request_send_async);

  if (!g_strcmp0 (uri->path, "addAccount")) {
	  add_account (result, params);
  } else if (!g_strcmp0 (uri->path, "getAccounts")) {
	  get_accounts (result, params);
  } else if (!g_strcmp0 (uri->path, "getMessages")) {
	  fetch_messages (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "syncAccount")) {
	  sync_account (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "runSendQueue")) {
	  run_send_queue (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "syncOutboxStore")) {
	  sync_outbox_store (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "getMessage")) {
	  get_message (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "composerSend")) {
	  composer_save (result, params, TRUE, cancellable);
  } else if (!g_strcmp0 (uri->path, "composerSaveDraft")) {
	  composer_save (result, params, FALSE, cancellable);
  } else if (!g_strcmp0 (uri->path, "flagMessage")) {
	  flag_message (result, params, cancellable);
  } else if (!g_strcmp0 (uri->path, "openFileURI")) {
	  open_file_uri (result, params);
  } else {
	  const gchar *callback_id;
	  callback_id = g_hash_table_lookup (params, "callback");
	  if (_error == NULL)
		  g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			       _("Non supported method"));
	  response_finish (result, callback_id, NULL, _error);
  }

  if (_error)
	  g_error_free (_error);

  g_hash_table_unref (params);
  g_object_unref (result);
}

static GInputStream *
im_soup_request_send_finish (SoupRequest          *soup_request,
			     GAsyncResult         *result,
			     GError              **error)
{
	g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);

	return (GInputStream *) g_object_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

static goffset
im_soup_request_get_content_length (SoupRequest *request)
{
  return  IM_SOUP_REQUEST (request)->priv->content_length;
}

static const char *
im_soup_request_get_content_type (SoupRequest *request)
{
  return "application/json";
}

static const char *request_schemes[] = { IM_SCHEME, NULL };

static void
im_soup_request_class_init (ImSoupRequestClass *request_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (request_class);
  SoupRequestClass *soup_request_class = SOUP_REQUEST_CLASS (request_class);

  gobject_class->finalize = im_soup_request_finalize;

  soup_request_class->schemes = request_schemes;
  soup_request_class->check_uri = im_soup_request_check_uri;
  soup_request_class->send_async = im_soup_request_send_async;
  soup_request_class->send_finish = im_soup_request_send_finish;
  soup_request_class->get_content_length = im_soup_request_get_content_length;
  soup_request_class->get_content_type = im_soup_request_get_content_type;

  g_type_class_add_private (request_class, sizeof (ImSoupRequestPrivate));
}
