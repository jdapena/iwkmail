/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-mail-ops.c : Asynchronous mail operations */

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

#include "im-error.h"
#include "im-mail-ops.h"

#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <string.h>

typedef struct _ComposerSaveAsyncContext {
	CamelMimeMessage *message;
	gchar *body;
	GList *attachment_uris;
	gchar *uid;
} ComposerSaveAsyncContext;

static void
composer_save_async_context_free (ComposerSaveAsyncContext *context)
{
	g_object_unref (context->message);
	g_free (context->body);
	g_list_foreach (context->attachment_uris, (GFunc) g_free, NULL);
	g_list_free (context->attachment_uris);
	g_free (context->uid);
}

/**
 * im_mail_op_composer_save_sync:
 * @folder: a #CamelFolder
 * @message: a #CamelMimeMessage
 * @body: a string with the plain text body
 * @attachment_uris: (element-type utf-8): list of attachment URIS
 * @uid: (out) (allow-none): the UID of the appended message.
 * @cancellable: optional #GCancellable object, or %NULL,
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Appends @message to @folder, creating its parts structure
 * from @body and @attachment_uris.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
gboolean
im_mail_op_composer_save_sync (CamelFolder *folder,
			       CamelMimeMessage *message,
			       const gchar *body,
			       GList *attachment_uris,
			       gchar **uid,
			       GCancellable *cancellable,
			       GError **error)
{
	GError *_error = NULL;

	if (attachment_uris) {
		CamelMultipart *multipart;
		CamelMimePart *body_part;
		GList *node;

		multipart = camel_multipart_new ();
		camel_data_wrapper_set_mime_type (CAMEL_DATA_WRAPPER (multipart), "multipart/mixed");
		camel_multipart_set_boundary (multipart, NULL);

		body_part = camel_mime_part_new ();
		camel_mime_part_set_content (CAMEL_MIME_PART (body_part), body, strlen (body), "text/plain; charset=utf8");
		camel_multipart_add_part (multipart, body_part);

		for (node = attachment_uris; _error == NULL && node != NULL; node = g_list_next (node)) {
			CamelMimePart *part = camel_mime_part_new ();
			CamelStream *content_stream;
			SoupURI *uri;
			const char *path;

			uri = soup_uri_new ((char *) node->data);
			path = soup_uri_get_path (uri);
			camel_mime_part_set_disposition (part, "attachment");
			if (path) {
				gchar *filename = g_path_get_basename (path);
				camel_mime_part_set_filename (part, filename);
				g_free (filename);
			}
			soup_uri_free (uri);

			content_stream = camel_stream_vfs_new_with_uri ((gchar *) node->data, CAMEL_STREAM_VFS_READ);
			if (content_stream) {
				CamelDataWrapper *content;

				content = camel_data_wrapper_new ();
				if (camel_data_wrapper_construct_from_stream_sync (content, 
										    content_stream, 
										    cancellable,
										    &_error)) {
					camel_medium_set_content (CAMEL_MEDIUM (part), content);
				}

				g_object_unref (content);
			} else {
				g_set_error (&_error, IM_ERROR_DOMAIN,
					     IM_ERROR_SEND_INVALID_ATTACHMENT,
					     _("Failed to get attachment data"));
			}

			if (_error == NULL)
				camel_multipart_add_part (multipart, part);
			g_object_unref (part);
		}

		camel_medium_set_content (CAMEL_MEDIUM (message), CAMEL_DATA_WRAPPER (multipart));
		g_object_unref (multipart);
		g_object_unref (body_part);
	} else if (body && *body) {
		camel_mime_part_set_content (CAMEL_MIME_PART (message), body, strlen (body), "text/plain; charset=utf8");
	}

	if (_error == NULL) {
		CamelMessageInfo *mi;

		mi = camel_message_info_new (NULL);
		camel_folder_append_message_sync (folder, message, mi, uid,
						  cancellable, &_error);
	}

	if (_error)
		g_propagate_error (error, _error);

	return _error == NULL;
}

static void
im_mail_op_composer_save_thread (GSimpleAsyncResult *simple,
				 GObject *object,
				 GCancellable *cancellable)
{
	GError *_error = NULL;
	ComposerSaveAsyncContext *context;

	context = (ComposerSaveAsyncContext *)
		g_simple_async_result_get_op_res_gpointer (simple);

	im_mail_op_composer_save_sync (CAMEL_FOLDER (object),
				       context->message, 
				       context->body,
				       context->attachment_uris,
				       &(context->uid),
				       cancellable,
				       &_error);

	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
				 
}

/**
 * im_mail_op_composer_save_async:
 * @folder: a #CamelFolder
 * @message: a #CamelMimeMessage
 * @body: a string with the plain text body
 * @attachment_uris: (element-type utf-8): list of attachment URIS
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Asynchronously appends @message to @folder, creating its parts structure
 * from @body and @attachment_uris.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_composer_save_finish() to get the result of the operation.
 */
void
im_mail_op_composer_save_async (CamelFolder *folder,
				CamelMimeMessage *message,
				const gchar *body,
				GList *attachment_uris,
				int io_priority,
				GCancellable *cancellable,
				GAsyncReadyCallback callback,
				gpointer userdata)
{
	GSimpleAsyncResult *simple;
	ComposerSaveAsyncContext *context;
	GList *node;

	context = g_new0 (ComposerSaveAsyncContext, 1);
	context->message = g_object_ref (message);
	context->body = g_strdup (body);
	context->attachment_uris = NULL;
	for (node = attachment_uris; node != NULL; node = g_list_next (node)) {
		context->attachment_uris = g_list_prepend (context->attachment_uris,
							   g_strdup (node->data));
	}
	context->attachment_uris = g_list_reverse (context->attachment_uris);
	context->uid = NULL;

	simple = g_simple_async_result_new (G_OBJECT (folder),
					    callback, userdata,
					    im_mail_op_composer_save_async);

	g_simple_async_result_set_op_res_gpointer (simple, context, 
						   (GDestroyNotify) composer_save_async_context_free);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_composer_save_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}

/**
 * im_mail_op_composer_save_finish:
 * @folder: a #CamelFolder
 * @result: a #GAsyncResult
 * @uid: (out) (allow-none): the uid of the appended message.
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_composer_save_async().
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
im_mail_op_composer_save_finish (CamelFolder *folder,
				 GAsyncResult *result,
				 gchar **uid,
				 GError **error)
{
	GSimpleAsyncResult *simple;
	ComposerSaveAsyncContext *context;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (folder), im_mail_op_composer_save_async), FALSE);

	simple = G_SIMPLE_ASYNC_RESULT (result);
	context = g_simple_async_result_get_op_res_gpointer (simple);

	if (uid != NULL) {
		*uid = context->uid;
		context->uid = NULL;
	}

	return !g_simple_async_result_propagate_error (simple, error);
}
