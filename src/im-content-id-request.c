/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-content-id-request.c : SoupRequest implementing cid: protocol */

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
#include "im-content-id-request.h"

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

G_DEFINE_TYPE (ImContentIdRequest, im_content_id_request, SOUP_TYPE_REQUEST)

struct _ImContentIdRequestPrivate {
	gssize content_length;
	gchar *content_type;
};

static void
im_content_id_request_init (ImContentIdRequest *request)
{
  request->priv = G_TYPE_INSTANCE_GET_PRIVATE (request, IM_TYPE_CONTENT_ID_REQUEST, ImContentIdRequestPrivate);
  request->priv->content_length = 0;
  request->priv->content_type = NULL;
}

static void
im_content_id_request_finalize (GObject *obj)
{
  G_OBJECT_CLASS (im_content_id_request_parent_class)->finalize (obj);
}

static void
escape_dots (gchar **part)
{
	char **parts;

	parts = g_strsplit (*part, ".", -1);
	g_free (*part);
	*part = g_strjoinv ("%2e", parts);
	g_strfreev (parts);
}

/* Our format for absolute content id's is: */
/* messageuid.folder.account */

gchar *
im_content_id_request_build_hostname (const char *account,
				      const char *folder,
				      const char *messageuid)
{
	gchar *escaped_account;
	gchar *escaped_folder;
	gchar *escaped_messageuid;
	gchar *result;

	escaped_account = g_uri_escape_string (account, NULL, FALSE);
	escaped_folder = g_uri_escape_string (folder, NULL, FALSE);
	escaped_messageuid = g_uri_escape_string (messageuid, NULL, FALSE);

	escape_dots (&escaped_account);
	escape_dots (&escaped_folder);
	escape_dots (&escaped_messageuid);

	result = g_strconcat (escaped_messageuid, ".", escaped_folder, ".", escaped_account, NULL);

	g_free (escaped_account);
	g_free (escaped_folder);
	g_free (escaped_messageuid);

	return result;
}

static void
get_content_id_hostname_parts (const char *cid_hostname,
			       gchar **account,
			       gchar **folder,
			       gchar **messageuid)
{
	gchar **parts = g_strsplit (cid_hostname, ".", 3);
	gchar *_account, *_folder, *_messageuid;

	if (parts[0] != NULL && parts[1] != NULL && parts[2] != NULL) {
		_account = g_uri_unescape_string (parts[2], NULL);
		_folder = g_uri_unescape_string (parts[1], NULL);
		_messageuid = g_uri_unescape_string (parts[0], NULL);
	} else {
		_account = _folder = _messageuid = NULL;
	}
	if (account)
		*account = _account;
	else
		g_free (_account);
	if (folder)
		*folder = _folder;
	else
		g_free (_folder);
	if (messageuid)
		*messageuid = _messageuid;
	else
		g_free (_messageuid);
}

static gboolean
im_content_id_request_check_uri (SoupRequest  *request,
				 SoupURI      *uri,
				 GError      **error)
{
	char *account, *folder, *messageuid;
	GError *_error = NULL;

	get_content_id_hostname_parts (uri->host, &account, &folder, &messageuid);

	if (!im_account_mgr_account_exists (im_account_mgr_get_instance (), account, FALSE)) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			     _("Invalid CID URI hostname"));
	}
	if (_error == NULL && atoi(messageuid) < 0) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			     _("Invalid CID URI message uid"));
	}

	if (_error) {
		g_propagate_error (error, _error);
	}
	return (_error == NULL);
}

typedef struct _FetchPartData {
	ImContentIdRequest *request;
	GAsyncResult *result;
	SoupURI *uri;
	GCancellable *cancellable;
	GError *error;
	GByteArray *byte_array;
	gchar *jsonp_callback;
} FetchPartData;

static void
fetch_part_finish (FetchPartData *data)
{
	g_free (data->jsonp_callback);
	if (data->error != NULL) {
		g_simple_async_result_take_error (G_SIMPLE_ASYNC_RESULT (data->result), data->error);
	}
	g_simple_async_result_complete_in_idle (G_SIMPLE_ASYNC_RESULT (data->result));
	soup_uri_free (data->uri);
	if (data->byte_array)
		g_byte_array_free (data->byte_array, TRUE);
	g_object_unref (data->cancellable);
	g_object_unref (data->request);
	g_free (data);
}

static CamelDataWrapper *
find_with_content_id (CamelDataWrapper *wrapper, const char *path)
{
	gchar *content_id;
	content_id = g_strrstr (path, "/");
	if (content_id == NULL)
		return NULL;
	content_id++;
	if (CAMEL_IS_MIME_PART (wrapper) &&
	    g_strcmp0 (camel_mime_part_get_content_id (CAMEL_MIME_PART (wrapper)),
		       content_id) == 0) {
		return camel_medium_get_content (CAMEL_MEDIUM (wrapper));
	} else if (CAMEL_IS_MEDIUM (wrapper)) {
		return find_with_content_id (camel_medium_get_content (CAMEL_MEDIUM (wrapper)), path);
	} else if (CAMEL_IS_MULTIPART (wrapper)) {
		gint count, i;
		count = camel_multipart_get_number (CAMEL_MULTIPART (wrapper));
		for (i = 0; i < count; i++) {
			CamelDataWrapper *found;
			found = find_with_content_id ((CamelDataWrapper *) camel_multipart_get_part (CAMEL_MULTIPART (wrapper), i), path);
			if (found) {
				return found;
			}
		}
	}
	return NULL;
}

static CamelDataWrapper *
find_part (CamelDataWrapper *wrapper, const char *path)
{
	if (path == NULL || *path == '\0') {
		if (CAMEL_IS_MEDIUM (wrapper))
			return camel_medium_get_content (CAMEL_MEDIUM (wrapper));
		else
			return wrapper;
	} else if (CAMEL_IS_MEDIUM (wrapper)) {
		CamelDataWrapper *content;

		content = camel_medium_get_content (CAMEL_MEDIUM (wrapper));
		if (CAMEL_IS_MULTIPART (content)) {
			const char *number_pos;
			char *next_path;
			int index;

			number_pos = path;
			if (g_str_has_prefix (number_pos, "/"))
				number_pos++;
			index = strtol (number_pos, &next_path, 10);
			if (next_path != number_pos && index >= 0 &&
			    index < camel_multipart_get_number (CAMEL_MULTIPART (content)))
				return find_part ((CamelDataWrapper *) camel_multipart_get_part (CAMEL_MULTIPART (content), index), next_path);
		}
	}

	return NULL;
}

static void
fetch_part_decode_to_stream_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	FetchPartData *data = (FetchPartData *) userdata;
	GError *error = NULL;
	CamelDataWrapper *data_wrapper = (CamelDataWrapper *) source_object;

	camel_data_wrapper_decode_to_stream_finish (data_wrapper, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (!error) {
		GInputStream *input_stream;

		if (data->jsonp_callback) {
			JsonBuilder *builder;
			JsonGenerator *generator;
			gchar *array_data;
			gchar *json_data;
			gchar *result;

			builder = json_builder_new ();
			json_builder_begin_object (builder);
			json_builder_set_member_name (builder, "data");
			array_data = g_strndup ((char *) data->byte_array->data, data->byte_array->len);
			json_builder_add_string_value (builder, array_data);
			g_free (array_data);
			json_builder_end_object (builder);

			generator = json_generator_new ();
			json_generator_set_root (generator, json_builder_get_root (builder));
			json_data = json_generator_to_data (generator, NULL);
			g_object_unref (generator);
			g_object_unref (builder);

			result = g_strdup_printf ("%s (%s)", data->jsonp_callback, json_data);
			g_free (json_data);
			g_byte_array_free (data->byte_array, TRUE);
			data->byte_array = g_byte_array_new_take ((guint8 *) result, strlen (result));
		}

		data->request->priv->content_length = data->byte_array->len;

		input_stream = g_memory_input_stream_new_from_data (g_byte_array_free (data->byte_array, FALSE), data->request->priv->content_length, g_free);
		data->byte_array = NULL;

		g_object_ref (input_stream);
		g_simple_async_result_set_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (data->result),
							   input_stream,
							   g_object_unref);
	} else if (data->error == NULL) {
		g_set_error (&data->error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			     _("Message not available"));
	}

	fetch_part_finish (data);
}

static void
normalize_content_type (CamelContentType *content_type)
{
	gchar *new_type;
	gchar *new_subtype;

	new_type = g_ascii_strdown (content_type->type, -1);
	new_subtype = g_ascii_strdown (content_type->subtype, -1);

	g_free (content_type->type);
	g_free (content_type->subtype);

	content_type->type = new_type;
	content_type->subtype = new_subtype;
}

static void
fetch_part_get_message_cb (GObject *source_object,
			   GAsyncResult *result,
			   gpointer userdata)
{
	FetchPartData *data = (FetchPartData *) userdata;
	GError *error = NULL;
	CamelFolder *folder = (CamelFolder *) source_object;
	CamelMimeMessage *message;

	message = camel_folder_get_message_finish (folder, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (message) {
		CamelDataWrapper *wrapper;
		wrapper = find_with_content_id (CAMEL_DATA_WRAPPER (message), data->uri->path);
		if (!wrapper)
			wrapper = find_part (CAMEL_DATA_WRAPPER (message), data->uri->path);
		if (wrapper == NULL) {
			g_set_error (&data->error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
				     _("Part not available"));
		} else {
			CamelStream *stream;
			CamelContentType *content_type;

			data->byte_array = g_byte_array_new ();
			stream = camel_stream_mem_new ();
			content_type = camel_data_wrapper_get_mime_type_field (CAMEL_DATA_WRAPPER (wrapper));
			normalize_content_type (content_type);
			data->request->priv->content_type = camel_content_type_format (content_type);

			camel_stream_mem_set_byte_array (CAMEL_STREAM_MEM (stream), data->byte_array);
			camel_data_wrapper_decode_to_stream (CAMEL_DATA_WRAPPER (wrapper),
							     stream,
							     G_PRIORITY_DEFAULT_IDLE, data->cancellable,
							     fetch_part_decode_to_stream_cb,
							     data);
		}
	} else if (data->error == NULL) {
		g_set_error (&data->error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			     _("Message not available"));
	}

	if (data->error != NULL)
		fetch_part_finish (data);
}

static void
fetch_part_get_folder_cb (GObject *source_object,
			  GAsyncResult *result,
			  gpointer userdata)
{
	FetchPartData *data = (FetchPartData *) userdata;
	GError *error = NULL;
	CamelStore *store = (CamelStore *) source_object;
	CamelFolder *folder;

	folder = camel_store_get_folder_finish (store, result, &error);

	if (error && data->error == NULL) {
		g_cancellable_cancel (data->cancellable);
		g_propagate_error (&data->error, error);
	}

	if (data->error == NULL) {
		char *messageuid;

		get_content_id_hostname_parts (data->uri->host, NULL, NULL, &messageuid);
		camel_folder_get_message (folder, messageuid,
					  G_PRIORITY_DEFAULT_IDLE, data->cancellable,
					  fetch_part_get_message_cb, data);
		g_free (messageuid);
	} else {
		fetch_part_finish (data);
	}
}


static void
im_content_id_request_send_async (SoupRequest          *soup_request,
				  GCancellable         *cancellable,
				  GAsyncReadyCallback   callback,
				  gpointer              userdata)
{
	FetchPartData *data;
	SoupURI *uri = soup_request_get_uri (SOUP_REQUEST (soup_request));
	char *account, *folder;
	CamelStore *store;

	get_content_id_hostname_parts (uri->host, &account, &folder, NULL);

	data = g_new0 (FetchPartData, 1);

	if (soup_uri_get_query (uri)) {
		GHashTable *params = soup_form_decode (soup_uri_get_query (uri));
		data->jsonp_callback = g_strdup (g_hash_table_lookup (params, "callback"));
	}

	data->request = g_object_ref (soup_request);
	data->result = (GAsyncResult *) g_simple_async_result_new ((GObject *) soup_request,
								   callback, userdata,
								   im_content_id_request_send_async);
	data->uri = soup_uri_copy (uri);
	data->cancellable = g_object_ref (cancellable);
	
	store = (CamelStore *) im_service_mgr_get_service (im_service_mgr_get_instance (),
							   (const char *) account,
							   IM_ACCOUNT_TYPE_STORE);
	if (store == NULL) {
		g_set_error (&data->error, IM_ERROR_DOMAIN, IM_ERROR_SOUP_INVALID_URI,
			     _("Account does not exist"));
	}
	
	g_free (account);

	if (data->error == NULL)
		camel_store_get_folder (store, folder,
					CAMEL_STORE_FOLDER_CREATE | CAMEL_STORE_FOLDER_BODY_INDEX,
					G_PRIORITY_DEFAULT_IDLE, data->cancellable,
					fetch_part_get_folder_cb, data);
	else
		fetch_part_finish (data);
	
	g_free (folder);
}

static GInputStream *
im_content_id_request_send_finish (SoupRequest          *soup_request,
				   GAsyncResult         *result,
				   GError              **error)
{
	g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);

	return (GInputStream *) g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
}

static goffset
im_content_id_request_get_content_length (SoupRequest *request)
{
  return  IM_CONTENT_ID_REQUEST (request)->priv->content_length;
}

static const char *
im_content_id_request_get_content_type (SoupRequest *request)
{
	ImContentIdRequest *cid_request = (ImContentIdRequest *) request;
	return cid_request->priv->content_type;
}

static const char *request_schemes[] = { IM_CONTENT_ID_SCHEME, NULL };

static void
im_content_id_request_class_init (ImContentIdRequestClass *request_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (request_class);
  SoupRequestClass *soup_request_class = SOUP_REQUEST_CLASS (request_class);

  gobject_class->finalize = im_content_id_request_finalize;

  soup_request_class->schemes = request_schemes;
  soup_request_class->check_uri = im_content_id_request_check_uri;
  soup_request_class->send_async = im_content_id_request_send_async;
  soup_request_class->send_finish = im_content_id_request_send_finish;
  soup_request_class->get_content_length = im_content_id_request_get_content_length;
  soup_request_class->get_content_type = im_content_id_request_get_content_type;

  g_type_class_add_private (request_class, sizeof (ImContentIdRequestPrivate));
}
