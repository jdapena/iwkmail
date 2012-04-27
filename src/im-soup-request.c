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
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_TYPE_INBOX);
		json_builder_set_member_name (builder, "isOutbox");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_TYPE_OUTBOX);
		json_builder_set_member_name (builder, "isSent");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_TYPE_SENT);
		json_builder_set_member_name (builder, "isTrash");
		json_builder_add_boolean_value (builder, fi->flags & CAMEL_FOLDER_TYPE_TRASH);
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
finish_sync_folders (SyncFoldersData *data)
{
	GList *account_keys, *node;
	json_builder_set_member_name (data->builder, "result");
	json_builder_begin_array (data->builder);

	account_keys = g_hash_table_get_keys (data->folder_infos);
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

static void
sync_folders_refresh_info_cb (GObject *source_object,
			      GAsyncResult *result,
			      gpointer userdata)
{
	SyncFoldersData *data = (SyncFoldersData *) userdata;
	data->count--;
	if (camel_folder_refresh_info_finish (CAMEL_FOLDER (source_object),
					      result,
					      NULL)) {
		CamelStore *store;
		char *account_id;

		store = camel_folder_get_parent_store (CAMEL_FOLDER (source_object));
		account_id = im_account_mgr_get_server_parent_account_name (im_account_mgr_get_instance (),
									    camel_service_get_uid (CAMEL_SERVICE (store)),
									    IM_ACCOUNT_TYPE_STORE);
		g_hash_table_insert (g_hash_table_lookup (data->updated_folders, account_id), 
				     g_strdup (camel_folder_get_full_name (CAMEL_FOLDER (source_object))),
				     g_object_ref (source_object));
	}

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
	GError *error = NULL;

	data->count--;

	folder = camel_store_get_folder_finish (CAMEL_STORE (source_object),
						result, &error);
	if (folder) {
		data->count++;
		camel_folder_refresh_info (folder,
					   G_PRIORITY_DEFAULT_IDLE,
					   data->cancellable,
					   sync_folders_refresh_info_cb,
					   data);
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
		g_hash_table_insert (data->folder_infos, g_strdup (account_id), fi);
		g_hash_table_insert (data->updated_folders, g_strdup (account_id),
				     g_hash_table_new_full (g_str_hash, g_str_equal,
							    g_free, (GDestroyNotify) g_object_unref));
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

		  store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
								     (const char *) node->data,
								     IM_ACCOUNT_TYPE_STORE);

		  if (CAMEL_IS_STORE (store)) {
			  g_hash_table_insert (data->stores, g_strdup ((char*) node->data), g_object_ref (store));
							       
			  camel_store_get_folder_info (store, NULL,
						       CAMEL_STORE_FOLDER_INFO_RECURSIVE |
						       CAMEL_STORE_FOLDER_INFO_NO_VIRTUAL |
						       CAMEL_STORE_FOLDER_INFO_SUBSCRIBED,
						       G_PRIORITY_DEFAULT_IDLE,
						       data->cancellable,
						       sync_folders_get_folder_info_cb,
						       data);
			  data->count++;
		  }
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

	response_start (data->builder);
	camel_folder_refresh_info_finish (folder, result, &error);

	uids = camel_folder_get_uids (folder);
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
	camel_folder_free_uids (folder, uids);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}
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

static void
im_soup_request_send_async (SoupRequest          *soup_request,
			    GCancellable         *cancellable,
			    GAsyncReadyCallback   callback,
			    gpointer              userdata)
{
  ImSoupRequest *request = IM_SOUP_REQUEST (soup_request);
  SoupURI *uri = soup_request_get_uri (SOUP_REQUEST (request));
  GHashTable *params;
  GError *_error = NULL;
  JsonBuilder *builder = json_builder_new ();
  GAsyncResult *result;

  params = soup_form_decode (soup_uri_get_query (uri));
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
  }

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
