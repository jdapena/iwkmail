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
#include "im-protocol-registry.h"
#include "im-server-account-settings.h"
#include "im-service-mgr.h"

#include <camel/camel.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <libsoup/soup-uri.h>

#define IM_OUTBOX_SEND_STATUS "iwk-send-status"
#define IM_OUTBOX_SEND_STATUS_COPYING_TO_SENTBOX "copying-to-sentbox"
#define IM_OUTBOX_SEND_STATUS_FAILED "failed"
#define IM_OUTBOX_SEND_STATUS_RETRY "retry"
#define IM_OUTBOX_SEND_STATUS_SEND "send"
#define IM_OUTBOX_SEND_STATUS_SENDING "sending"
#define IM_OUTBOX_SEND_STATUS_SENT "sent"
#define IM_OUTBOX_SEND_ATTEMPTS "iwk-send-attempts"
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
	const char *password;

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
	password = (const char *) g_hash_table_lookup (params, "incoming-server-password");

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
	im_server_account_settings_set_password (settings, password);
	im_server_account_settings_set_security_protocol (settings, im_protocol_get_type_id (security_protocol));

finish:
	if (protocol) g_object_unref (protocol);
	if (security_protocol) g_object_unref (protocol);
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
	const char *password;

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
	password = (const char *) g_hash_table_lookup (params, "outgoing-server-password");

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
	im_server_account_settings_set_password (settings, password);
	im_server_account_settings_set_security_protocol (settings, im_protocol_get_type_id (security_protocol));
	im_server_account_settings_set_auth_protocol (settings, im_protocol_get_type_id (auth_protocol));

finish:
	g_object_unref (protocol);
	if (security_protocol) g_object_unref (protocol);
	if (auth_protocol) g_object_unref (auth_protocol);
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
response_start (JsonBuilder *builder)
{
  json_builder_begin_object (builder);

}

static void
response_finish (GAsyncResult *result, GHashTable *params, JsonBuilder *builder, GError *error)
{
	ImSoupRequest *request;
	JsonGenerator *generator;
	char *json_data;
	gssize length;
	GString *buffer = g_string_new (NULL);
	const char *callback;
	GInputStream *input_stream;

	request = (ImSoupRequest *) g_async_result_get_source_object (result);
	
	callback = (char *) g_hash_table_lookup (params, "callback");

	if (callback)
		g_string_append_printf (buffer, "%s (", callback);

	json_builder_set_member_name (builder, "is_ok");
	json_builder_add_boolean_value (builder, error == NULL);
	if (error) {
		json_builder_set_member_name (builder, "error");
		json_builder_add_string_value (builder, error->message);
	}
	json_builder_end_object (builder);

	generator = json_generator_new ();
	json_generator_set_root (generator, json_builder_get_root (builder));
	json_data = json_generator_to_data (generator, NULL);
	g_object_unref (generator);

	g_string_append (buffer, json_data);
	g_free (json_data);

	if (callback)
		g_string_append (buffer, ")");

	length = buffer->len;

	input_stream = g_memory_input_stream_new_from_data (g_string_free (buffer, FALSE),
							    length,
							    g_free);
	g_simple_async_result_set_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result),
						   input_stream,
						   g_object_unref);
	g_hash_table_unref (params);

	request->priv->content_length = length;

	g_simple_async_result_complete_in_idle (G_SIMPLE_ASYNC_RESULT (result));
}

static void
add_account (GHashTable *request_params,
	     JsonBuilder *builder,
	     GError **error)
{
	GError *_error = NULL;
	ImAccountSettings *account;
	ImServerAccountSettings *store, *transport;
	GHashTable *params;
	const char *form_data;

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
	if (store) g_object_unref (store);
	if (_error) g_propagate_error (error, _error);
}

static void
get_accounts (GHashTable *request_params,
	      JsonBuilder *builder,
	      GError **error)
{
	  GSList *account_names, *node;
	  ImAccountMgr *mgr = im_account_mgr_get_instance ();
	  char *default_account;

	  account_names = im_account_mgr_account_names (mgr, TRUE);
	  default_account = im_account_mgr_get_default_account (mgr);

	  json_builder_set_member_name (builder, "result");
	  json_builder_begin_array (builder);
	  for (node = account_names; node != NULL; node = g_slist_next (node)) {
		  char *id = (char *) node->data;
		  ImAccountSettings *settings;

		  settings = im_account_mgr_load_account_settings (mgr, id);

		  json_builder_begin_object (builder);

		  json_builder_set_member_name (builder, "id");
		  json_builder_add_string_value (builder, id);

		  json_builder_set_member_name (builder, "displayName");
		  json_builder_add_string_value (builder, im_account_settings_get_display_name (settings));

		  json_builder_set_member_name (builder, "emailAddress");
		  json_builder_add_string_value (builder, im_account_settings_get_email_address (settings));

		  json_builder_set_member_name (builder, "isDefault");
		  json_builder_add_boolean_value (builder, im_account_settings_get_is_default (settings));

		  g_object_unref (settings);

		  json_builder_end_object (builder);
	  }
	  json_builder_end_array (builder);

	  g_free (default_account);
	  im_account_mgr_free_account_names (account_names);
}

typedef struct _SyncFoldersData {
	GAsyncResult *result;
	GHashTable *params;
	JsonBuilder *builder;
	GCancellable *cancellable;
	GError *error;
	gint count;
	GHashTable *folder_infos;
	GHashTable *stores;
	GHashTable *updated_folders;
} SyncFoldersData;

static void
dump_folder_info (JsonBuilder *builder,
		  CamelFolderInfo *fi,
		  GHashTable *account_updated_folders)
{
	while (fi) {
		CamelFolder *folder = g_hash_table_lookup (account_updated_folders,
							   fi->full_name);
		json_builder_set_member_name (builder, fi->full_name);
		json_builder_begin_object (builder);
		json_builder_set_member_name (builder, "fullName");
		json_builder_add_string_value (builder, fi->full_name);
		json_builder_set_member_name (builder, "displayName");
		json_builder_add_string_value (builder, fi->display_name);
		if (folder) {
			json_builder_set_member_name (builder, "unreadCount");
			json_builder_add_int_value (builder,
						    camel_folder_get_unread_message_count (folder));
			json_builder_set_member_name (builder, "messageCount");
			json_builder_add_int_value (builder,
						    camel_folder_get_message_count (folder));
		} else {
			json_builder_set_member_name (builder, "unreadCount");
			json_builder_add_int_value (builder, fi->unread);
			json_builder_set_member_name (builder, "messageCount");
			json_builder_add_int_value (builder, fi->total);
		}
		json_builder_set_member_name (builder, "noSelect");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_NOSELECT);
		json_builder_set_member_name (builder, "favourite");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_CHECK_FOR_NEW);
		json_builder_set_member_name (builder, "isInbox");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_INBOX);
		json_builder_set_member_name (builder, "isOutbox");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_OUTBOX);
		json_builder_set_member_name (builder, "isSent");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_SENT);
		json_builder_set_member_name (builder, "isTrash");
		json_builder_add_boolean_value (builder, (fi->flags & CAMEL_FOLDER_TYPE_MASK) == CAMEL_FOLDER_TYPE_TRASH);
		if (fi->parent) {
			json_builder_set_member_name (builder, "parentFullName");
			json_builder_add_string_value (builder, fi->parent->full_name);
		}
		json_builder_end_object (builder);
		if (fi->child) {
			dump_folder_info (builder, fi->child, account_updated_folders);
		}
		fi = fi->next;
	}
}

static void
dump_inbox_folder (JsonBuilder *builder,
		   CamelFolder *folder)
{
	json_builder_set_member_name (builder, camel_folder_get_full_name (folder));
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "fullName");
	json_builder_add_string_value (builder, camel_folder_get_full_name (folder));
	json_builder_set_member_name (builder, "displayName");
	json_builder_add_string_value (builder, camel_folder_get_display_name (folder));
	json_builder_set_member_name (builder, "unreadCount");
	json_builder_add_int_value (builder,
				    camel_folder_get_unread_message_count (folder));
	json_builder_set_member_name (builder, "messageCount");
	json_builder_add_int_value (builder,
				    camel_folder_get_message_count (folder));
	json_builder_set_member_name (builder, "noSelect");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "favourite");
	json_builder_add_boolean_value (builder, TRUE);
	json_builder_set_member_name (builder, "isInbox");
	json_builder_add_boolean_value (builder, TRUE);
	json_builder_set_member_name (builder, "isOutbox");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isSent");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_set_member_name (builder, "isTrash");
	json_builder_add_boolean_value (builder, FALSE);
	json_builder_end_object (builder);
}

static void
finish_sync_folders (SyncFoldersData *data)
{
	GList *account_keys, *node;
	json_builder_set_member_name (data->builder, "result");
	json_builder_begin_array (data->builder);

	account_keys = g_hash_table_get_keys (data->updated_folders);
	for (node = account_keys; node != NULL; node = g_list_next (node)) {
		char *account_id = (char *) node->data;
		char *display_name;
		CamelFolderInfo *fi;
		GHashTable *updated_folders;
		json_builder_begin_object (data->builder);

		json_builder_set_member_name (data->builder, "accountId");
		json_builder_add_string_value (data->builder, account_id);

		json_builder_set_member_name (data->builder, "accountName");
		display_name = im_account_mgr_get_display_name (im_account_mgr_get_instance (),
								account_id);
		json_builder_add_string_value (data->builder, display_name);

		json_builder_set_member_name (data->builder, "folders");
		json_builder_begin_object (data->builder);
		fi = g_hash_table_lookup (data->folder_infos, account_id);

		updated_folders = g_hash_table_lookup (data->updated_folders, account_id);
		if (fi) {	
			CamelStore *store = g_hash_table_lookup (data->stores, account_id);
			dump_folder_info (data->builder, fi, updated_folders);
			camel_store_free_folder_info (store, fi);
		} else {
			/* No hierarchy, we just dump the existing folder */
			GList *folders;
			folders = g_hash_table_get_keys (updated_folders);
			if (folders) {
				CamelFolder *inbox = g_hash_table_lookup (updated_folders,
									  folders->data);
				if (inbox)
					dump_inbox_folder (data->builder, inbox);
				g_list_free (folders);
			}
		}
		json_builder_end_object (data->builder);
		json_builder_end_object (data->builder);

		g_free (display_name);
	}
	json_builder_end_array (data->builder);

	g_hash_table_destroy (data->stores);
	g_hash_table_destroy (data->folder_infos);
	g_hash_table_destroy (data->updated_folders);
	response_finish (data->result,
			 data->params,
			 data->builder,
			 data->error);

	g_object_unref (data->result);
	g_hash_table_unref (data->params);
	g_object_unref (data->builder);
	if (data->error) g_error_free (data->error);
	g_object_unref (data->cancellable);

	g_free (data);
}

typedef struct _OutboxGetMessageData {
	SyncFoldersData *data;
	CamelFolder *outbox;
	char *uid;
} OutboxGetMessageData;

static void
sync_folders_outbox_send_to_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	OutboxGetMessageData *get_message_data = (OutboxGetMessageData *) userdata;
	SyncFoldersData *data = get_message_data->data;
	CamelFolder *outbox = get_message_data->outbox;
	GError *_error = NULL;
	char *uid = get_message_data->uid;

	data->count--;
	if (!camel_transport_send_to_finish (CAMEL_TRANSPORT (source_object), result, &_error)) {
		const char *attempt_str;
		int attempt;

		attempt_str = camel_folder_get_message_user_tag (outbox, uid,
								 IM_OUTBOX_SEND_ATTEMPTS);
		attempt = attempt_str?atoi (attempt_str):0;
		if (attempt > 3 || attempt < 0) {
			camel_folder_set_message_user_tag (outbox, uid,
							   IM_OUTBOX_SEND_STATUS,
							   IM_OUTBOX_SEND_STATUS_FAILED);
		} else {
			char *new_attempt;
			camel_folder_set_message_user_tag (outbox, uid,
							   IM_OUTBOX_SEND_STATUS,
							   IM_OUTBOX_SEND_STATUS_RETRY);
			new_attempt = g_strdup_printf ("%d", attempt + 1);
			camel_folder_set_message_user_tag (outbox, uid,
							   IM_OUTBOX_SEND_ATTEMPTS,
							   new_attempt);
			g_free (new_attempt);
		}
	} else {
		camel_folder_set_message_user_tag (outbox, uid,
						   IM_OUTBOX_SEND_ATTEMPTS,
						   "0");
		camel_folder_set_message_user_tag (outbox, uid,
						   IM_OUTBOX_SEND_STATUS,
						   IM_OUTBOX_SEND_STATUS_SENT);
	}
	camel_folder_synchronize_sync (outbox, FALSE, NULL, NULL);

	g_free (get_message_data->uid);
	g_object_unref (get_message_data->outbox);
	g_free (get_message_data);

	if (_error) {
		g_propagate_error (&data->error, _error);
	}

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_outbox_get_message_cb (GObject *source_object,
				    GAsyncResult *result,
				    gpointer userdata)
{
	OutboxGetMessageData *get_message_data = (OutboxGetMessageData *) userdata;
	SyncFoldersData *data = get_message_data->data;
	CamelFolder *outbox = (CamelFolder *) source_object;
	GError *_error = NULL;
	char *uid = get_message_data->uid;

	CamelMimeMessage *message;

	data->count--;

	message = camel_folder_get_message_finish (outbox, result, &_error);

	if (_error || message == NULL) {
		camel_folder_set_message_user_tag (outbox, uid,
						   IM_OUTBOX_SEND_STATUS,
						   IM_OUTBOX_SEND_STATUS_RETRY);

		camel_folder_synchronize_sync (outbox, FALSE, NULL, NULL);
		g_free (get_message_data->uid);
		g_object_unref (get_message_data->outbox);
		g_free (get_message_data);

	} else {
		CamelTransport *transport;
		CamelInternetAddress *recipients;
			
		transport = (CamelTransport *)
			im_service_mgr_get_service (im_service_mgr_get_instance (),
						    camel_folder_get_full_name (outbox),
						    IM_ACCOUNT_TYPE_TRANSPORT);

		recipients = camel_internet_address_new ();
		camel_address_cat (CAMEL_ADDRESS (recipients),
				   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_TO)));
		camel_address_cat (CAMEL_ADDRESS (recipients),
				   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_CC)));
		camel_address_cat (CAMEL_ADDRESS (recipients),
				   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_BCC)));
		
		if (camel_service_connect_sync (CAMEL_SERVICE (transport),
						&_error)) {
			camel_transport_send_to (transport,
						 message, CAMEL_ADDRESS (camel_mime_message_get_from (message)),
						     CAMEL_ADDRESS (recipients),
						 G_PRIORITY_DEFAULT_IDLE, data->cancellable,
						 sync_folders_outbox_send_to_cb, get_message_data);
			data->count++;
		}
		
		g_object_unref (recipients);
	}

	if (_error) {
		g_propagate_error (&data->error, _error);
	}

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_outbox_synchronize_cb (GObject *source_object,
				    GAsyncResult *result,
				    gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	CamelFolder *outbox = (CamelFolder *) source_object;
	data->count--;
	if (camel_folder_synchronize_finish (CAMEL_FOLDER (source_object),
					     result,
					     NULL)) {

		if (camel_folder_get_message_count (outbox) > 0) {
			CamelTransport *transport;

			/* Get transport */
			transport = (CamelTransport *)
				im_service_mgr_get_service (im_service_mgr_get_instance (),
							    camel_folder_get_full_name (outbox),
							    IM_ACCOUNT_TYPE_TRANSPORT);

			if (transport) {
				GPtrArray *uids;
				gint i;

				uids = camel_folder_get_uids (outbox);

				for (i = 0; i < uids->len; i++) {
					const char *uid = (const char *) uids->pdata[i];
					const char *send_status;
					send_status = camel_folder_get_message_user_tag (outbox, uid, IM_OUTBOX_SEND_STATUS);
					if (g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_SENDING) == 0 ||
					    g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_COPYING_TO_SENTBOX) == 0) {
						/* we ignore it, it's being operated now */
					} else if (g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_SENT) == 0) {
						/* TODO: it's sent, but transfer to sent folder failed, we reschedule it */
					} else if (send_status == NULL || g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_RETRY) == 0){
						OutboxGetMessageData *get_message_data = g_new0 (OutboxGetMessageData, 1);
						camel_folder_set_message_user_tag (outbox, uid,
										   IM_OUTBOX_SEND_STATUS,
										   IM_OUTBOX_SEND_STATUS_SENDING);
						camel_folder_synchronize_sync (outbox, FALSE, NULL, NULL);
						get_message_data->data = data;
						get_message_data->uid = g_strdup (uid);
						get_message_data->outbox = g_object_ref (outbox);
						camel_folder_get_message (outbox, uid,
									  G_PRIORITY_DEFAULT_IDLE,
									  data->cancellable,
									  sync_folders_outbox_get_message_cb,
									  get_message_data);
						data->count++;
					}
				}
			}
		}
	}

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_refresh_info_cb (GObject *source_object,
			      GAsyncResult *result,
			      gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	GError *_error = NULL;

	data->count--;
	if (camel_folder_refresh_info_finish (CAMEL_FOLDER (source_object),
					      result,
					      &_error)) {
		CamelStore *store;
		char *account_id;

		store = camel_folder_get_parent_store (CAMEL_FOLDER (source_object));
		account_id = im_account_mgr_get_server_parent_account_name (im_account_mgr_get_instance (),
									    camel_service_get_uid (CAMEL_SERVICE (store)),
									    IM_ACCOUNT_TYPE_STORE);
		if (account_id)
			g_hash_table_insert (g_hash_table_lookup (data->updated_folders, account_id), 
					     g_strdup (camel_folder_get_full_name (CAMEL_FOLDER (source_object))),
					     g_object_ref (source_object));
	}

	if (_error)
		g_error_free (_error);

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_get_folder_cb (GObject *source_object,
			    GAsyncResult *result,
			    gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	CamelFolder *folder;
	GError *_error = NULL;

	data->count--;

	folder = camel_store_get_folder_finish (CAMEL_STORE (source_object),
						result, &_error);
	if (folder) {
		if (camel_service_get_connection_status (CAMEL_SERVICE (source_object)) ==
		    CAMEL_SERVICE_CONNECTED ||
		    camel_service_connect_sync (CAMEL_SERVICE (source_object), &_error)) {
			data->count++;
			camel_folder_refresh_info (folder,
						   G_PRIORITY_DEFAULT_IDLE,
						   data->cancellable,
						   sync_folders_refresh_info_cb,
						   data);
		}
	}
	if (_error)
		g_error_free (_error);

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_get_pop_inbox_cb (GObject *source_object,
			       GAsyncResult *result,
			       gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	CamelFolder *folder;
	GError *_error = NULL;

	data->count--;

	folder = camel_store_get_inbox_folder_finish (CAMEL_STORE (source_object),
						      result, &_error);
	if (folder) {
		if (camel_service_get_connection_status (CAMEL_SERVICE (source_object)) ==
		    CAMEL_SERVICE_CONNECTED ||
		    camel_service_connect_sync (CAMEL_SERVICE (source_object), &_error)) {
			data->count++;
			camel_folder_refresh_info (folder,
						   G_PRIORITY_DEFAULT_IDLE,
						   data->cancellable,
						   sync_folders_refresh_info_cb,
						   data);
		}
	}

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders_get_folder_info_cb (GObject *source_object,
		 GAsyncResult *res,
		 gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	CamelFolderInfo *fi;
	GError *error = NULL;
	gchar *account_id;
	data->count--;

	account_id = im_account_mgr_get_server_parent_account_name (im_account_mgr_get_instance (),
								    camel_service_get_uid (CAMEL_SERVICE (source_object)),
								    IM_ACCOUNT_TYPE_STORE);
	fi = camel_store_get_folder_info_finish (CAMEL_STORE (source_object),
						 res, &error);
	if (fi) {
		if (account_id) {
			g_hash_table_insert (data->folder_infos, g_strdup (account_id), fi);
			g_hash_table_insert (data->updated_folders, g_strdup (account_id),
					     g_hash_table_new_full (g_str_hash, g_str_equal,
								    g_free, (GDestroyNotify) g_object_unref));
		}
		if (camel_store_can_refresh_folder (CAMEL_STORE (source_object), fi, NULL)) {
			camel_store_get_folder (CAMEL_STORE (source_object),
						fi->full_name,
						CAMEL_STORE_FOLDER_CREATE |
						CAMEL_STORE_FOLDER_BODY_INDEX,
						G_PRIORITY_DEFAULT_IDLE,
						data->cancellable,
						sync_folders_get_folder_cb,
						data);
			data->count++;
		}
	}
	g_free (account_id);
	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (data->count == 0) {
		finish_sync_folders (data);
	}
}

static void
sync_folders (GAsyncResult *result, GHashTable *params, JsonBuilder *builder)
{
	  GSList *account_names, *node;
	  ImAccountMgr *mgr = im_account_mgr_get_instance ();
	  SyncFoldersData *data = g_new0(SyncFoldersData, 1);
	  CamelStore *outbox_store;

	  response_start (builder);

	  data->result = g_object_ref (result);
	  data->params = g_hash_table_ref (params);
	  data->builder = g_object_ref (builder);
	  data->cancellable = g_cancellable_new ();
	  data->stores = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, (GDestroyNotify) g_object_unref);
	  data->folder_infos = g_hash_table_new_full (g_str_hash, g_str_equal,
						      g_free, NULL);
	  data->updated_folders = g_hash_table_new_full (g_str_hash, g_str_equal,
							 g_free, (GDestroyNotify) g_hash_table_destroy);

	  account_names = im_account_mgr_account_names (mgr, TRUE);

	  for (node = account_names; node != NULL; node = g_slist_next (node)) {
		  CamelStore *store;
		  CamelFolder *outbox;

		  store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
								     (const char *) node->data,
								     IM_ACCOUNT_TYPE_STORE);

		  if (CAMEL_IS_STORE (store)) {
			  CamelProvider *provider = camel_service_get_provider (CAMEL_SERVICE (store));
			  g_hash_table_insert (data->stores, g_strdup ((char*) node->data), g_object_ref (store));
			  if (g_strcmp0 (provider->protocol, "pop") == 0) {
				  g_hash_table_insert (data->updated_folders, g_strdup ((char *) node->data),
						       g_hash_table_new_full (g_str_hash, g_str_equal,
									      g_free, (GDestroyNotify) g_object_unref));
				  camel_store_get_inbox_folder (store,
								G_PRIORITY_DEFAULT_IDLE,
								data->cancellable,
								sync_folders_get_pop_inbox_cb,
								data);
			  } else {
				  camel_store_get_folder_info (store, NULL,
							       CAMEL_STORE_FOLDER_INFO_RECURSIVE |
							       CAMEL_STORE_FOLDER_INFO_NO_VIRTUAL |
							       CAMEL_STORE_FOLDER_INFO_SUBSCRIBED,
							       G_PRIORITY_DEFAULT_IDLE,
							       data->cancellable,
							       sync_folders_get_folder_info_cb,
							       data);
			  }
			  data->count++;
		  }
		  outbox = im_service_mgr_get_outbox (im_service_mgr_get_instance (),
						      (const char *)node->data,
						      data->cancellable,
						      NULL);
		  if (outbox) {
			  camel_folder_synchronize (outbox, TRUE,
						    G_PRIORITY_DEFAULT_IDLE,
						    data->cancellable,
						    sync_folders_outbox_synchronize_cb,
						    data);
			  data->count++;
		  }
		  
	  }

	  outbox_store = im_service_mgr_get_outbox_store (im_service_mgr_get_instance ());
	  if (outbox_store) {
		  camel_store_get_folder_info (outbox_store, NULL,
					       CAMEL_STORE_FOLDER_INFO_RECURSIVE |
					       CAMEL_STORE_FOLDER_INFO_NO_VIRTUAL |
					       CAMEL_STORE_FOLDER_INFO_SUBSCRIBED,
					       G_PRIORITY_DEFAULT_IDLE,
					       data->cancellable,
					       sync_folders_get_folder_info_cb,
					       data);
		  data->count++;
	  }
	  if (data->count == 0)
		  finish_sync_folders (data);
}

typedef struct _FetchMessagesData {
	GAsyncResult *result;
	GHashTable *params;
	JsonBuilder *builder;
	GCancellable *cancellable;
	GError *error;
	GList *new_uids;
	GList *old_uids;
	GHashTable *messages;
	int count;
} FetchMessagesData;

static void
dump_message_info (JsonBuilder *builder,
		   CamelMessageInfo *mi)
{
	CamelMessageFlags flags;
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
	json_builder_set_member_name (builder, "mailingList");
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

	flags = camel_message_info_flags (mi);
	json_builder_set_member_name (builder, "unread");
	json_builder_add_boolean_value (builder,
					!(flags & CAMEL_MESSAGE_SEEN));
	flags = camel_message_info_flags (mi);
	json_builder_set_member_name (builder, "draft");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_DRAFT);
	json_builder_set_member_name (builder, "hasAttachments");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_ATTACHMENTS);
	json_builder_end_object(builder);
}

static void
dump_internet_address_as_string (JsonBuilder *builder, CamelInternetAddress *cia)
{
	char *format;

	format = camel_address_format (CAMEL_ADDRESS (cia));
	json_builder_add_string_value (builder, format);
	g_free (format);
}

static void
dump_message_as_info (JsonBuilder *builder,
		      CamelFolder *folder,
		      const char *uid,
		      CamelMimeMessage *message)
{
	CamelMessageFlags flags;
	json_builder_begin_object(builder);
	json_builder_set_member_name (builder, "uid");
	json_builder_add_string_value (builder, uid);

	json_builder_set_member_name (builder, "from");
	dump_internet_address_as_string (builder, camel_mime_message_get_from (message));
	json_builder_set_member_name (builder, "to");
	dump_internet_address_as_string (builder, camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_TO));
	json_builder_set_member_name (builder, "cc");
	dump_internet_address_as_string (builder, camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_CC));

	json_builder_set_member_name (builder, "dateReceived");
	json_builder_add_int_value (builder,
				    camel_mime_message_get_date_received (message, NULL));
	json_builder_set_member_name (builder, "dateSent");
	json_builder_add_int_value (builder,
				    camel_mime_message_get_date (message, NULL));

	json_builder_set_member_name (builder, "subject");
	json_builder_add_string_value (builder,
				       camel_mime_message_get_subject (message));

	flags = camel_folder_get_message_flags (folder, uid);
	json_builder_set_member_name (builder, "unread");
	json_builder_add_boolean_value (builder,
					!(flags & CAMEL_MESSAGE_SEEN));
	json_builder_set_member_name (builder, "draft");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_DRAFT);
	json_builder_set_member_name (builder, "hasAttachments");
	json_builder_add_boolean_value (builder,
					flags & CAMEL_MESSAGE_ATTACHMENTS);
	json_builder_end_object(builder);
}

typedef struct _FetchMessagesGetMessageData {
	FetchMessagesData *data;
	gchar *uid;
} FetchMessagesGetMessageData;

static void
fetch_messages_pop3_get_message_cb (GObject *source_object,
				    GAsyncResult *result,
				    gpointer userdata)
{
	FetchMessagesGetMessageData *get_message_data = (FetchMessagesGetMessageData *) userdata;
	FetchMessagesData *data = get_message_data->data;
	GError *_error = NULL;
	CamelFolder *folder = (CamelFolder *) source_object;
	CamelMimeMessage *message;

	data->count--;

	message = camel_folder_get_message_finish (folder,
						   result,
						   &_error);

	g_hash_table_insert (data->messages,
			     g_strdup (get_message_data->uid),
			     message);

	g_free (get_message_data->uid);
	g_free (get_message_data);

	if (_error)
		g_error_free (_error);

	if (data->count == 0) {
		GList *node;
		json_builder_set_member_name (data->builder, "newMessages");
		json_builder_begin_array (data->builder);
		for (node = data->new_uids; node != NULL; node = g_list_next (node)) {
			message = g_hash_table_lookup (data->messages, (char *) node->data);
			if (message) {
				dump_message_as_info (data->builder, folder,
						      (char *) node->data, message);
			}
		}
		json_builder_end_array (data->builder);
		json_builder_set_member_name (data->builder, "messages");
		json_builder_begin_array (data->builder);
		for (node = data->old_uids; node != NULL; node = g_list_next (node)) {
			message = g_hash_table_lookup (data->messages, (char *) node->data);
			if (message) {
				dump_message_as_info (data->builder, folder,
						      (char *) node->data, message);
			}
		}
		json_builder_end_array (data->builder);
		response_finish (data->result, data->params, data->builder, data->error);
	}
}

static void
fetch_messages_refresh_info_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	FetchMessagesData *data = (FetchMessagesData *) userdata;
	GError *error = NULL;
	CamelFolder *folder = (CamelFolder *) source_object;
	const char *newest_uid;
	const char *oldest_uid;
	const char *count_str;
	gint count;
	GPtrArray *uids;
	int i, j;

	newest_uid = g_hash_table_lookup (data->params, "newestUid");
	if (newest_uid == '\0' || g_strcmp0 (newest_uid, "null") == 0) newest_uid = NULL;
	oldest_uid = g_hash_table_lookup (data->params, "oldestUid");
	if (oldest_uid == '\0' || g_strcmp0 (oldest_uid, "null") == 0) oldest_uid = NULL;
	count_str = g_hash_table_lookup (data->params, "count");
	if (count_str) {
		count = atoi(count_str);
	}
	if (count < 0)
		count = 20;

	camel_folder_refresh_info_finish (folder, result, &error);

	data->count = 0;

	uids = camel_folder_get_uids (folder);
	response_start (data->builder);
	if (!camel_folder_has_summary_capability (folder)) {

		data->messages = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

		i = uids->len - 1;
		if (newest_uid != NULL) {
			while (i >= 0) {
				FetchMessagesGetMessageData *get_message_data;
				const char *uid = uids->pdata[i];
				if (g_strcmp0 (uid, newest_uid) == 0)
					break;
				data->new_uids = g_list_append (data->new_uids, g_strdup (uid));
				get_message_data = g_new0 (FetchMessagesGetMessageData, 1);
				get_message_data->data = data;
				get_message_data->uid = g_strdup (uid);
				camel_folder_get_message (folder, uid,
							  G_PRIORITY_DEFAULT_IDLE,
							  data->cancellable,
							  fetch_messages_pop3_get_message_cb,
							  get_message_data);
				data->count++;
				i--;
			}
		}
		if (oldest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				if (g_strcmp0 (uid, oldest_uid) == 0)
					break;
				i--;
			}
			i--;
		}
		for (j = 0; j < count; j++) {
			FetchMessagesGetMessageData *get_message_data;
			const char *uid;
			if (i < 0)
				break;
			uid = (const char *) uids->pdata[i];
			data->old_uids = g_list_append (data->old_uids, g_strdup (uid));
			get_message_data = g_new0 (FetchMessagesGetMessageData, 1);
			get_message_data->data = data;
			get_message_data->uid = g_strdup (uid);
			camel_folder_get_message (folder, uid,
						  G_PRIORITY_DEFAULT_IDLE,
						  data->cancellable,
						  fetch_messages_pop3_get_message_cb,
						  get_message_data);
			data->count++;
			i--;
		}

		if (data->count == 0) {
			json_builder_set_member_name (data->builder, "newMessages");
			json_builder_begin_array(data->builder);
			json_builder_end_array (data->builder);
			json_builder_set_member_name (data->builder, "messages");
			json_builder_begin_array (data->builder);
			json_builder_end_array (data->builder);
		}
	} else {
		camel_folder_sort_uids (folder, uids);
		
		/* Fetch first new messages */
		json_builder_set_member_name (data->builder, "newMessages");
		json_builder_begin_array(data->builder);
		i = uids->len - 1;
		if (newest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				CamelMessageInfo *mi;
				if (g_strcmp0 (uid, newest_uid) == 0)
					break;
				mi = camel_folder_get_message_info (folder, uid);
				dump_message_info (data->builder, mi);
				camel_folder_free_message_info (folder, mi);
				i--;
			}
		}
		json_builder_end_array (data->builder);
		if (oldest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				if (g_strcmp0 (uid, oldest_uid) == 0)
					break;
				i--;
			}
			i--;
		}
		json_builder_set_member_name (data->builder, "messages");
		json_builder_begin_array (data->builder);
		for (j = 0; j < count; j++) {
			const char *uid;
			CamelMessageInfo *mi;
			if (i < 0)
				break;
			uid = (const char *) uids->pdata[i];
			mi = camel_folder_get_message_info (folder, uid);
			dump_message_info (data->builder, mi);
			camel_folder_free_message_info (folder, mi);
			i--;
		}
		json_builder_end_array(data->builder);
	}
	camel_folder_free_uids (folder, uids);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (data->count == 0)
		response_finish (data->result, data->params, data->builder, data->error);
}

static void
fetch_messages_get_folder_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	FetchMessagesData *data = (FetchMessagesData *) userdata;
	GError *error = NULL;
	CamelStore *store = (CamelStore *) source_object;
	CamelFolder *folder;

	folder = camel_store_get_folder_finish (store, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (data->error == NULL) {
		camel_folder_refresh_info (folder, 
					   G_PRIORITY_DEFAULT_IDLE, data->cancellable,
					   fetch_messages_refresh_info_cb, data);
	}else {
		response_finish (data->result, data->params, data->builder, data->error);
	}
}

static void
fetch_messages (GAsyncResult *result, GHashTable *params, JsonBuilder *builder)
{
	CamelStore *store;
	const char *account_name;
	const char *folder_name;
	FetchMessagesData *data = g_new0 (FetchMessagesData, 1);

	data->result = g_object_ref (result);
	data->params = g_hash_table_ref (params);
	data->builder = g_object_ref (builder);
	data->cancellable = g_cancellable_new ();

	account_name = g_hash_table_lookup (params, "account");
	folder_name = g_hash_table_lookup (params, "folder");
	store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
							   (const char *) account_name,
							   IM_ACCOUNT_TYPE_STORE);
	camel_store_get_folder (store, folder_name,
				CAMEL_STORE_FOLDER_CREATE | CAMEL_STORE_FOLDER_BODY_INDEX, 
				G_PRIORITY_DEFAULT_IDLE, data->cancellable,
				fetch_messages_get_folder_cb, data);
}

typedef struct _GetMessageData {
	GAsyncResult *result;
	GHashTable *params;
	JsonBuilder *builder;
	GCancellable *cancellable;
	GError *error;
} GetMessageData;

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
get_message_get_message_cb (GObject *source_object,
			    GAsyncResult *result,
			    gpointer userdata)
{
	GetMessageData *data = (GetMessageData *) userdata;
	GError *error = NULL;
	CamelFolder *folder = (CamelFolder *) source_object;
	CamelMimeMessage *message;

	message = camel_folder_get_message_finish (folder, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	response_start (data->builder);
	if (message) {
		CamelURL *url;
		json_builder_set_member_name (data->builder, "result");
		url = build_message_url (g_hash_table_lookup (data->params, "account"),
					 g_hash_table_lookup (data->params, "folder"),
					 g_hash_table_lookup (data->params, "message"));
		dump_data_wrapper (data->builder, url, CAMEL_DATA_WRAPPER (message));
		camel_url_free (url);
	}

	response_finish (data->result, data->params, data->builder, data->error);
}

static void
get_message_get_folder_cb (GObject *source_object,
			   GAsyncResult *result,
			   gpointer userdata)
{
	GetMessageData *data = (GetMessageData *) userdata;
	GError *error = NULL;
	CamelStore *store = (CamelStore *) source_object;
	CamelFolder *folder;

	folder = camel_store_get_folder_finish (store, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (data->error == NULL) {
		const char *message_uid;

		message_uid = g_hash_table_lookup (data->params, "message");
		camel_folder_get_message (folder, message_uid,
					   G_PRIORITY_DEFAULT_IDLE, data->cancellable,
					   get_message_get_message_cb, data);
	} else {
		response_finish (data->result, data->params, data->builder, data->error);
	}
}

static void
get_message (GAsyncResult *result, GHashTable *params, JsonBuilder *builder)
{
	CamelStore *store;
	const char *account_name;
	const char *folder_name;
	GetMessageData *data = g_new0 (GetMessageData, 1);

	data->result = g_object_ref (result);
	data->params = g_hash_table_ref (params);
	data->builder = g_object_ref (builder);
	data->cancellable = g_cancellable_new ();

	account_name = g_hash_table_lookup (params, "account");
	folder_name = g_hash_table_lookup (params, "folder");
	store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
							   (const char *) account_name,
							   IM_ACCOUNT_TYPE_STORE);
	camel_store_get_folder (store, folder_name,
				CAMEL_STORE_FOLDER_CREATE | CAMEL_STORE_FOLDER_BODY_INDEX, 
				G_PRIORITY_DEFAULT_IDLE, data->cancellable,
				get_message_get_folder_cb, data);
}

typedef struct _ComposerSendData {
	GAsyncResult *result;
	GHashTable *params;
	JsonBuilder *builder;
	GCancellable *cancellable;
	GError *error;
	char *uid;
} ComposerSendData;

static void
finish_composer_send_data (ComposerSendData *data)
{
	response_start (data->builder);
	if (data->uid) {
		json_builder_set_member_name (data->builder, "outboxUid");
		json_builder_add_string_value (data->builder, data->uid);
	}
	response_finish (data->result, data->params, data->builder, data->error);

	g_object_unref (data->result);
	g_hash_table_unref (data->params);
	g_object_unref (data->builder);
	if (data->error) g_error_free (data->error);
	g_object_unref (data->cancellable);
	g_free (data);
}

static void
composer_send_append_message_cb (GObject *source_object,
				 GAsyncResult *result,
				 gpointer userdata)
{
	ComposerSendData *data = (ComposerSendData *) userdata;
	GError *_error = NULL;
	CamelFolder *outbox = (CamelFolder *) source_object;

	if (!camel_folder_append_message_finish (outbox, result, &data->uid, &_error) && _error == NULL) {
		g_set_error (&_error, IM_ERROR_DOMAIN,
			     IM_ERROR_SEND_FAILED_TO_ADD_TO_OUTBOX,
			     _("Couldn't add to outbox"));
	}

	if (_error)
		g_propagate_error (&data->error, _error);

	finish_composer_send_data (data);
}

static void
composer_send (GAsyncResult *result, GHashTable *params, JsonBuilder *builder)
{
	ComposerSendData *data = g_new0 (ComposerSendData, 1);
	CamelMimeMessage *message;
	CamelMessageInfo *mi;
	CamelFolder *outbox;
	const char *account_id;
	const char *to, *cc, *bcc;
	const char *subject;
	const char *body;
	GError *_error = NULL;
	const char *form_data;
	GHashTable *form_params;

	data->result = g_object_ref (result);
	data->builder = g_object_ref (builder);
	data->cancellable = g_cancellable_new ();
	data->params = g_hash_table_ref (params);

	form_data = g_hash_table_lookup (params, "formData");
	form_params = soup_form_decode (form_data);

	account_id = g_hash_table_lookup (form_params, "composer-from-choice");
	to = g_hash_table_lookup (form_params, "composer-to");
	cc = g_hash_table_lookup (form_params, "composer-cc");
	bcc = g_hash_table_lookup (form_params, "composer-bcc");
	subject = g_hash_table_lookup (form_params, "composer-subject");
	body = g_hash_table_lookup (form_params, "composer-body");

	message = camel_mime_message_new ();
	camel_medium_set_header (CAMEL_MEDIUM (message), "X-Mailer", IM_X_MAILER);
	camel_mime_message_set_subject (message, subject);
	camel_mime_message_set_date (message, CAMEL_MESSAGE_DATE_CURRENT, 0);

	if (body && *body)
		camel_mime_part_set_content (CAMEL_MIME_PART (message), body, strlen (body), "text/plain; charset=utf8");

	mi = camel_message_info_new (NULL);
	
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
		} else if (to_count + cc_count + bcc_count == 0) {
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
		outbox = im_service_mgr_get_outbox (im_service_mgr_get_instance (),
						    account_id,
						    data->cancellable,
						    &_error);
	}

	if (_error) {
		g_propagate_error (&data->error, _error);
	}

	g_hash_table_destroy (form_params);

	if (_error == NULL && outbox) {
		camel_folder_append_message (outbox, message, mi,
					     G_PRIORITY_DEFAULT_IDLE,
					     data->cancellable,
					     composer_send_append_message_cb, data);
	} else {
		finish_composer_send_data (data);
	}

	camel_message_info_free (mi);
	g_object_unref (message);
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
  JsonBuilder *builder = json_builder_new ();
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
	  response_start (builder);
	  add_account (params, builder, &_error);
	  response_finish (result, params, builder, _error);
  } else if (!g_strcmp0 (uri->path, "getAccounts")) {
	  response_start (builder);
	  get_accounts (params, builder, &_error);
	  response_finish (result, params, builder, _error);
  } else if (!g_strcmp0 (uri->path, "getMessages")) {
	  fetch_messages (result, params, builder);
  } else if (!g_strcmp0 (uri->path, "syncFolders")) {
	  sync_folders (result, params, builder);
  } else if (!g_strcmp0 (uri->path, "getMessage")) {
	  get_message (result, params, builder);
  } else if (!g_strcmp0 (uri->path, "composerSend")) {
	  composer_send (result, params, builder);
  } else {
	  response_start (builder);
	  if (_error == NULL)
		  g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			       _("Non supported method"));
	  response_finish (result, params, builder, _error);
  }

  if (_error)
	  g_error_free (_error);

  g_object_unref (builder);
}

static GInputStream *
im_soup_request_send_finish (SoupRequest          *soup_request,
			     GAsyncResult         *result,
			     GError              **error)
{
	g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);

	return (GInputStream *) g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
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
