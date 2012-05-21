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

#include "im-account-mgr-helpers.h"
#include "im-error.h"
#include "im-mail-ops.h"

#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <string.h>

#define IM_OUTBOX_SEND_STATUS "iwk-send-status"
#define IM_OUTBOX_SEND_STATUS_COPYING_TO_SENTBOX "copying-to-sentbox"
#define IM_OUTBOX_SEND_STATUS_FAILED "failed"
#define IM_OUTBOX_SEND_STATUS_RETRY "retry"
#define IM_OUTBOX_SEND_STATUS_SEND "send"
#define IM_OUTBOX_SEND_STATUS_SENDING "sending"
#define IM_OUTBOX_SEND_STATUS_SENT "sent"
#define IM_OUTBOX_SEND_ATTEMPTS "iwk-send-attempts"

static gboolean
run_send_queue_message_sync (CamelFolder *outbox,
			     CamelTransport *transport,
			     const gchar *uid,
			     GCancellable *cancellable,
			     GError **error)
{
	GError *_error = NULL;
	const char *send_status;

	send_status = camel_folder_get_message_user_tag (outbox, uid, IM_OUTBOX_SEND_STATUS);
	if (g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_SENDING) == 0 ||
	    g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_COPYING_TO_SENTBOX) == 0) {
		/* we ignore it, it's being operated now */
	} else if (g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_SENT) == 0) {
		/* TODO: it's sent, but transfer to sent folder failed, we reschedule it */
	} else if (send_status == NULL || g_strcmp0 (send_status, IM_OUTBOX_SEND_STATUS_RETRY) == 0){
		CamelMimeMessage *message = NULL;
		camel_folder_set_message_user_tag (outbox, uid,
						   IM_OUTBOX_SEND_STATUS,
						   IM_OUTBOX_SEND_STATUS_SENDING);
		camel_folder_synchronize_sync (outbox, FALSE, cancellable, &_error);
		if (_error == NULL) {
			message = camel_folder_get_message_sync (outbox, uid,
								 cancellable, &_error);
		}
		if (_error == NULL)
			camel_service_connect_sync (CAMEL_SERVICE (transport),
						    &_error);
		if (_error) {
			camel_folder_set_message_user_tag (outbox, uid,
							   IM_OUTBOX_SEND_STATUS,
							   IM_OUTBOX_SEND_STATUS_RETRY);
			camel_folder_synchronize_sync (outbox, FALSE, NULL, NULL);
		}
		if (_error == NULL) {
			CamelInternetAddress *recipients;
			recipients = camel_internet_address_new ();
			camel_address_cat (CAMEL_ADDRESS (recipients),
					   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_TO)));
			camel_address_cat (CAMEL_ADDRESS (recipients),
					   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_CC)));
			camel_address_cat (CAMEL_ADDRESS (recipients),
					   CAMEL_ADDRESS (camel_mime_message_get_recipients (message, CAMEL_RECIPIENT_TYPE_BCC)));
		
			if (!camel_transport_send_to_sync (transport,
							   message, CAMEL_ADDRESS (camel_mime_message_get_from (message)),
							   CAMEL_ADDRESS (recipients),
							   cancellable, &_error)) {
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
			g_object_unref (recipients);
		}
	}

	if (_error)
		g_propagate_error (error, _error);

	return _error == NULL;
}

/**
 * im_mail_op_run_send_queue_sync:
 * @outbox: an outbox #CamelFolder
 * @cancellable: optional #GCancellable object, or %NULL,
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
 *
 * Runs send queue for @outbox.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
gboolean
im_mail_op_run_send_queue_sync (CamelFolder *outbox,
				GCancellable *cancellable,
				GError **error)
{
	GError *_error = NULL;

	camel_folder_synchronize_sync (outbox, TRUE,
				       cancellable, &_error);
	if (_error == NULL && camel_folder_get_message_count (outbox) > 0) {
		CamelTransport *transport;

		/* Get transport */
		transport = (CamelTransport *)
			im_service_mgr_get_service (im_service_mgr_get_instance (),
						    camel_folder_get_full_name (outbox),
						    IM_ACCOUNT_TYPE_TRANSPORT);

		if (transport == NULL) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_INTERNAL,
				     _("No transport for an outbox"));
		}

		if (_error == NULL) {
			GPtrArray *uids;
			gint i;

			uids = camel_folder_get_uids (outbox);

			for (i = 0; i < uids->len; i++) {
				const char *uid = (const char *) uids->pdata[i];
				if (_error == NULL) {
					run_send_queue_message_sync (outbox, transport, uid, cancellable, &_error);
				}
			}
			camel_folder_free_uids (outbox, uids);
		}
	}

	if (_error) g_propagate_error (error, _error);

	return _error == NULL;
}

static void
im_mail_op_run_send_queue_thread (GSimpleAsyncResult *simple,
				  GObject *object,
				  GCancellable *cancellable)
{
	GError *_error = NULL;

	im_mail_op_run_send_queue_sync (CAMEL_FOLDER (object),
					cancellable,
					&_error);

	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
}

/**
 * im_mail_op_run_send_queue_async:
 * @outbox: an outbox #CamelFolder
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Runs send queue for @outbox.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_run_send_queue_finish() to get the result of the operation.
 */
void
im_mail_op_run_send_queue_async (CamelFolder *outbox,
				 int io_priority,
				 GCancellable *cancellable,
				 GAsyncReadyCallback callback,
				 gpointer userdata)
{
	GSimpleAsyncResult *simple;
	
	simple = g_simple_async_result_new (G_OBJECT (outbox),
					    callback, userdata,
					    im_mail_op_run_send_queue_async);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_run_send_queue_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}


/**
 * im_mail_op_run_send_queue_finish:
 * @outbox: a #CamelFolder
 * @result: a #GAsyncResult
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_run_send_queue_async().
 *
 * Returns: %TRUE if successfull, %FALSE otherwise.
 */
gboolean
im_mail_op_run_send_queue_finish (CamelFolder *outbox,
				  GAsyncResult *result,
				  GError **error)
{
	GSimpleAsyncResult *simple;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (outbox), im_mail_op_run_send_queue_async), FALSE);


	simple = G_SIMPLE_ASYNC_RESULT (result);
	return !g_simple_async_result_propagate_error (simple, error);
}

static gboolean
update_non_storage_uids_sync (CamelFolder *remote_inbox,
			      CamelFolder *local_inbox,
			      GCancellable *cancellable,
			      GError **error)
{
	GError *_error = NULL;
	const gchar *store_data_dir;
	gchar *uid_cache_path;
	CamelUIDCache *uid_cache;
	CamelStore *remote_store;

	remote_store = camel_folder_get_parent_store (remote_inbox);

	store_data_dir = camel_service_get_user_data_dir (CAMEL_SERVICE (remote_store));
	uid_cache_path = g_build_filename (store_data_dir, "remote-uid.cache", NULL);
	uid_cache = camel_uid_cache_new (uid_cache_path);
	g_free (uid_cache_path);

	if (uid_cache) {
		GPtrArray *remote_uids;
		GPtrArray *new_uids;

		remote_uids = camel_folder_get_uids (remote_inbox);
		new_uids = camel_uid_cache_get_new_uids (uid_cache, remote_uids);

		if (new_uids) {
			CamelFilterDriver *driver;

			driver = camel_filter_driver_new (CAMEL_SESSION (im_service_mgr_get_instance ()));
			camel_filter_driver_set_default_folder (driver,
								local_inbox);
			camel_filter_driver_filter_folder (driver, remote_inbox, 
							   uid_cache, new_uids, FALSE,
							   cancellable, &_error);
			camel_filter_driver_flush (driver, &_error);
			camel_uid_cache_save (uid_cache);

			camel_uid_cache_free_uids (new_uids);
			g_object_unref (driver);
		}

		camel_folder_free_uids (remote_inbox, remote_uids);
		camel_uid_cache_destroy (uid_cache);
	}

	if (_error) g_propagate_error (error, _error);
	return (_error != NULL);
}

static CamelFolderInfo *
synchronize_nonstorage_store_sync (CamelStore *store,
				   GCancellable *cancellable,
				   GError **error)
{
	GError *_error = NULL;
	CamelFolder *remote_inbox = NULL;
	CamelFolder *local_inbox = NULL;
	gchar *account_id;
	CamelFolderInfo *fi = NULL;

	remote_inbox = camel_store_get_inbox_folder_sync (store,
							  cancellable,
							  &_error);

	if (_error == NULL) {
		if (camel_service_get_connection_status (CAMEL_SERVICE (store)) ==
		    CAMEL_SERVICE_CONNECTED ||
		    camel_service_connect_sync (CAMEL_SERVICE (store), &_error)) {
			camel_folder_refresh_info_sync (remote_inbox,
							cancellable,
							&_error);
		} else {
			g_clear_error (&_error);
		}
	}

	if (_error == NULL) {
		account_id = im_account_mgr_get_server_parent_account_name (im_account_mgr_get_instance (),
									    camel_service_get_uid (CAMEL_SERVICE (store)),
									    IM_ACCOUNT_TYPE_STORE);
		if (account_id == NULL) {
			g_set_error (&_error, IM_ERROR_DOMAIN,
				     IM_ERROR_INTERNAL,
				     _("Could not find account of store"));
		}
	}

	if (_error == NULL) {
		local_inbox = im_service_mgr_get_local_inbox (im_service_mgr_get_instance (),
							      account_id,
							      cancellable,
							      &_error);
	}

	if (_error == NULL) {
		update_non_storage_uids_sync (remote_inbox, local_inbox,
					      cancellable, &_error);
	}

	if (_error == NULL) {
		CamelStore *local_store;

		local_store = im_service_mgr_get_local_store (im_service_mgr_get_instance ());
		fi = camel_store_get_folder_info_sync (local_store, account_id, 0,
						       cancellable, &_error);
	}

	g_free (account_id);
	if (local_inbox) g_object_unref (local_inbox);
	if (remote_inbox) g_object_unref (remote_inbox);

	return fi;
}

static CamelFolderInfo *
synchronize_storage_store_sync (CamelStore *store,
				GCancellable *cancellable,
				GError **error)
{
	GError *_error = NULL;
	CamelFolderInfo *fi = NULL;
	CamelFolder *folder = NULL;

	fi = camel_store_get_folder_info_sync (store, NULL,
					       CAMEL_STORE_FOLDER_INFO_RECURSIVE |
					       CAMEL_STORE_FOLDER_INFO_SUBSCRIBED,
					       cancellable,
					       &_error);

	if (_error == NULL) {
		if (camel_store_can_refresh_folder (store, fi, &_error)) {
			folder = camel_store_get_folder_sync (store,
							      fi->full_name,
							      CAMEL_STORE_FOLDER_CREATE |
							      CAMEL_STORE_FOLDER_BODY_INDEX,
							      cancellable,
							      &_error);

		}
	}

	if (folder && _error == NULL) {
		if (camel_service_get_connection_status (CAMEL_SERVICE (store)) ==
		    CAMEL_SERVICE_CONNECTED ||
		    camel_service_connect_sync (CAMEL_SERVICE (store), &_error)) {
			camel_folder_refresh_info_sync (folder,
							cancellable,
							&_error);
		}
	}

	if (folder && _error == NULL) {
		camel_folder_synchronize_sync (folder, FALSE,
					       cancellable, &_error);
	}

	if (folder) g_object_unref (folder);

	if (_error)
		g_propagate_error (error, _error);

	return fi;
}

/**
 * im_mail_op_synchronize_store_sync:
 * @store: a #CamelStore
 * @cancellable: optional #GCancellable object, or %NULL.
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
 *
 * Refreshes and obtains the folders structure for @store, and updates inbox.
 *
 * Returns: (transfer full): #CamelFolderInfo of the store if successful, %NULL otherwise.
 */
CamelFolderInfo *
im_mail_op_synchronize_store_sync (CamelStore *store,
				   GCancellable *cancellable,
				   GError **error)
{
	CamelProvider *provider;

	provider = camel_service_get_provider (CAMEL_SERVICE (store));
	if (provider->flags & CAMEL_PROVIDER_IS_STORAGE)
		return synchronize_storage_store_sync (store, cancellable, error);
	else
		return synchronize_nonstorage_store_sync (store, cancellable, error);

}

static void
im_mail_op_synchronize_store_thread (GSimpleAsyncResult *simple,
				     GObject *object,
				     GCancellable *cancellable)
{
	GError *_error = NULL;
	CamelFolderInfo *fi;

	fi = im_mail_op_synchronize_store_sync (CAMEL_STORE (object),
						cancellable,
						&_error);

	g_simple_async_result_set_op_res_gpointer (simple,
						   fi,
						   NULL);
	
	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
}

/**
 * im_mail_op_synchronize_store_async:
 * @store: a #CamelStore
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Asynchronously refreshes @store, fetching first the list of folders, and
 * then refreshing the inbox data.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_synchronize_storage_store_finish() to get the result of the operation.
 */
void
im_mail_op_synchronize_store_async (CamelStore *store,
				    int io_priority,
				    GCancellable *cancellable,
				    GAsyncReadyCallback callback,
				    gpointer userdata)
{
	GSimpleAsyncResult *simple;
	
	simple = g_simple_async_result_new (G_OBJECT (store),
					    callback, userdata,
					    im_mail_op_synchronize_store_async);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_synchronize_store_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}


/**
 * im_mail_op_synchronize_store_finish:
 * @store: a #CamelStore
 * @result: a #GAsyncResult
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_synchronize_store_async().
 *
 * Returns: a #CamelFolderInfo on success which should be freed with
 * camel_store_free_folder_info(), %NULL otherwise.
 */
CamelFolderInfo *
im_mail_op_synchronize_store_finish (CamelStore *store,
				     GAsyncResult *result,
				     GError **error)
{
	GSimpleAsyncResult *simple;
	CamelFolderInfo *fi;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (store), im_mail_op_synchronize_store_async), FALSE);


	simple = G_SIMPLE_ASYNC_RESULT (result);
	fi = (CamelFolderInfo *)
		g_simple_async_result_get_op_res_gpointer (simple);

	return fi;
}

typedef struct _RefreshFolderInfoAsyncContext {
	gchar *account_id;
	gchar *folder_name;
	CamelFolder *folder;
} RefreshFolderInfoAsyncContext;

static void
refresh_folder_info_async_context_free (RefreshFolderInfoAsyncContext *context)
{
	g_free (context->account_id);
	g_free (context->folder_name);
	if (context->folder) g_object_unref (context->folder);
	g_free (context);
}

/**
 * im_mail_op_refresh_folder_info_sync:
 * @mgr: a #ImServiceMgr
 * @account_id: an account id
 * @folder_name: a folder name
 * @folder: (out) (allow-none): the refreshed #CamelFolder
 * @cancellable: optional #GCancellable object, or %NULL.
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
 *
 * Refreshes the summary of folder @folder_name in account @account_id.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
gboolean
im_mail_op_refresh_folder_info_sync (ImServiceMgr *mgr,
				     const gchar *account_id,
				     const gchar *folder_name,
				     CamelFolder **folder,
				     GCancellable *cancellable,
				     GError **error)
{
	GError *_error = NULL;
	CamelFolder *_folder;

	_folder = im_service_mgr_get_folder (mgr, account_id,
					    folder_name, cancellable, &_error);

	if (_error == NULL) {
		camel_folder_refresh_info_sync (_folder,
						cancellable, &_error);
	}

	if (_error)
		g_propagate_error (error, _error);
	if (folder)
		*folder = _folder;
	else
		g_object_unref (_folder);

	return _error == NULL;
}

static void
im_mail_op_refresh_folder_info_thread (GSimpleAsyncResult *simple,
				       GObject *object,
				       GCancellable *cancellable)
{
	GError *_error = NULL;
	RefreshFolderInfoAsyncContext *context;

	context = (RefreshFolderInfoAsyncContext *)
		g_simple_async_result_get_op_res_gpointer (simple);

	im_mail_op_refresh_folder_info_sync (IM_SERVICE_MGR (object),
					     context->account_id,
					     context->folder_name,
					     &(context->folder),
					     cancellable,
					     &_error);
	
	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
}

/**
 * im_mail_op_refresh_folder_info_async:
 * @mgr: a #ImServiceMgr
 * @account_id: an account id
 * @folder_name: a folder name
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Asynchronously refreshes the summary of folder @folder_name in account
 * @account_id.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_refresh_folder_info_finish() to get the result of the operation.
 */
void
im_mail_op_refresh_folder_info_async (ImServiceMgr *mgr,
				      const gchar *account_id,
				      const gchar *folder_name,
				      int io_priority,
				      GCancellable *cancellable,
				      GAsyncReadyCallback callback,
				      gpointer userdata)
{
	GSimpleAsyncResult *simple;
	RefreshFolderInfoAsyncContext *context;

	context = g_new0 (RefreshFolderInfoAsyncContext, 1);
	context->account_id = g_strdup (account_id);
	context->folder_name = g_strdup (folder_name);

	simple = g_simple_async_result_new (G_OBJECT (mgr),
					    callback, userdata,
					    im_mail_op_refresh_folder_info_async);

	g_simple_async_result_set_op_res_gpointer (simple, context, 
						   (GDestroyNotify) refresh_folder_info_async_context_free);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_refresh_folder_info_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}

/**
 * im_mail_op_refresh_folder_info_finish:
 * @mgr: a #ImServiceMgr
 * @result: a #GAsyncResult
 * @folder: (out) (allow-none) (transfer full): refreshed #CamelFolder
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_refresh_folder_info_async().
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
im_mail_op_refresh_folder_info_finish (ImServiceMgr *mgr,
				       GAsyncResult *result,
				       CamelFolder **folder,
				       GError **error)
{
	GSimpleAsyncResult *simple;
	RefreshFolderInfoAsyncContext *context;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (mgr), im_mail_op_refresh_folder_info_async), FALSE);


	simple = G_SIMPLE_ASYNC_RESULT (result);
	context = (RefreshFolderInfoAsyncContext *)
		g_simple_async_result_get_op_res_gpointer (simple);

	if (folder && context->folder)
		*folder = g_object_ref (context->folder);

	return !g_simple_async_result_propagate_error (simple, error);
}

typedef struct _GetMessageAsyncContext {
	gchar *account_id;
	gchar *folder_name;
	gchar *message_uid;
	CamelMimeMessage *message;
} GetMessageAsyncContext;

static void
get_message_async_context_free (GetMessageAsyncContext *context)
{
	g_free (context->account_id);
	g_free (context->folder_name);
	g_free (context->message_uid);
	if (context->message) g_object_unref (context->message);
	g_free (context);
}

/**
 * im_mail_op_get_message_sync:
 * @account_id: an account id
 * @folder_name: a folder name
 * @message_uid: a message uid
 * @cancellable: optional #GCancellable object, or %NULL.
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
 *
 * Obtains the message with @message_uid from folder @folder_name
 * in account @account_id.
 *
 * Returns: (transfer full): a #CamelMimeMessage if successful, %NULL otherwise.
 */
CamelMimeMessage *
im_mail_op_get_message_sync (ImServiceMgr *service_mgr,
			     const gchar *account_id,
			     const gchar *folder_name,
			     const gchar *message_uid,
			     GCancellable *cancellable,
			     GError **error)
{
	GError *_error = NULL;
	CamelFolder *folder;
	CamelMimeMessage *message = NULL;

	folder = im_service_mgr_get_folder (service_mgr, account_id,
					    folder_name, cancellable, &_error);

	if (_error == NULL) {
		message = camel_folder_get_message_sync (folder, message_uid,
							 cancellable, &_error);
	}

	if (_error)
		g_propagate_error (error, _error);
	if (folder) g_object_unref (folder);

	return message;
}

static void
im_mail_op_get_message_thread (GSimpleAsyncResult *simple,
			       GObject *object,
			       GCancellable *cancellable)
{
	GError *_error = NULL;
	GetMessageAsyncContext *context;

	context = (GetMessageAsyncContext *)
		g_simple_async_result_get_op_res_gpointer (simple);

	context->message = im_mail_op_get_message_sync (IM_SERVICE_MGR (object),
							context->account_id,
							context->folder_name,
							context->message_uid,
							cancellable,
							&_error);
	
	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
}

/**
 * im_mail_op_get_message_async:
 * @mgr: a #ImServiceMgr
 * @account_id: an account id
 * @folder_name: a folder name
 * @message_uid: a message uid
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Asynchronously obtains the message with @message_uid from folder @folder_name
 * in account @account_id.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_get_message_finish() to get the result of the operation.
 */
void
im_mail_op_get_message_async (ImServiceMgr *mgr,
			       const gchar *account_id,
			       const gchar *folder_name,
			       const gchar *message_uid,
			       int io_priority,
			       GCancellable *cancellable,
			       GAsyncReadyCallback callback,
			       gpointer userdata)
{
	GSimpleAsyncResult *simple;
	GetMessageAsyncContext *context;

	context = g_new0 (GetMessageAsyncContext, 1);
	context->account_id = g_strdup (account_id);
	context->folder_name = g_strdup (folder_name);
	context->message_uid = g_strdup (message_uid);

	simple = g_simple_async_result_new (G_OBJECT (mgr),
					    callback, userdata,
					    im_mail_op_get_message_async);

	g_simple_async_result_set_op_res_gpointer (simple, context, 
						   (GDestroyNotify) get_message_async_context_free);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_get_message_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}

/**
 * im_mail_op_get_message_finish:
 * @folder: a #ImServiceMgr
 * @result: a #GAsyncResult
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_composer_get_message_async().
 *
 * Returns: (transfer full): a #CamelMimeMessage on success, %NULL otherwise.
 */
CamelMimeMessage *
im_mail_op_get_message_finish (ImServiceMgr *mgr,
			       GAsyncResult *result,
			       GError **error)
{
	GSimpleAsyncResult *simple;
	GetMessageAsyncContext *context;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (mgr), im_mail_op_get_message_async), FALSE);

	simple = G_SIMPLE_ASYNC_RESULT (result);
	context = g_simple_async_result_get_op_res_gpointer (simple);

	g_simple_async_result_propagate_error (simple, error);
	return context->message?g_object_ref (context->message):NULL;
}


typedef struct _MessageFlagAsyncContext {
	gchar *account_id;
	gchar *folder_name;
	gchar *message_uid;
	gchar *set_flags;
	gchar *unset_flags;
} FlagMessageAsyncContext;

static void
flag_message_async_context_free (FlagMessageAsyncContext *context)
{
	g_free (context->account_id);
	g_free (context->folder_name);
	g_free (context->message_uid);
	g_free (context->set_flags);
	g_free (context->unset_flags);
	g_free (context);
}

static CamelMessageFlags
parse_flags (const char *flags_list, GList **user_flags)
{
	gchar **flags, **node;
	CamelMessageFlags result = 0;

	*user_flags = NULL;
	if (flags_list == NULL)
		return 0;

	flags = g_strsplit (flags_list, ",", 0);
	for (node = flags; *node != NULL; node++) {
		if (g_strstr_len (*node, -1, "seen"))
			result |= CAMEL_MESSAGE_SEEN;
		else if (g_strstr_len (*node, -1, "deleted"))
			result |= CAMEL_MESSAGE_DELETED;
		else
			*user_flags = g_list_append (*user_flags, g_strdup (*node));
	}

	g_strfreev (flags);

	return result;
}

/**
 * im_mail_op_flag_message_sync:
 * @account_id: an account id
 * @folder_name: a folder name
 * @message_uid: a message uid
 * @set_flags: a string with the list of flags to set
 * @unset_flags: a string with the list of flags to unset
 * @cancellable: optional #GCancellable object, or %NULL.
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
 *
 * Modifies the flags and user flags of the message with @message_uid 
 * from folder @folder_name in account @account_id.
 *
 * Returns: %TRUE if successful, %NULL otherwise.
 */
gboolean
im_mail_op_flag_message_sync (ImServiceMgr *service_mgr,
			      const gchar *account_id,
			      const gchar *folder_name,
			      const gchar *message_uid,
			      const gchar *set_flags,
			      const gchar *unset_flags,
			      GCancellable *cancellable,
			      GError **error)
{
	GError *_error = NULL;
	CamelFolder *folder;

	folder = im_service_mgr_get_folder (service_mgr, account_id,
					    folder_name, cancellable, &_error);

	if (_error == NULL) {
		CamelMessageFlags camel_set_flags, camel_unset_flags;
		GList *unset_user_flags, *set_user_flags, *node;

		camel_set_flags = parse_flags (set_flags, &set_user_flags);
		camel_unset_flags = parse_flags (unset_flags, &unset_user_flags);

		if (camel_unset_flags)
			camel_folder_set_message_flags (folder, message_uid, 0, camel_unset_flags);
		if (camel_set_flags)
			camel_folder_set_message_flags (folder, message_uid, camel_set_flags, camel_set_flags);

		for (node = unset_user_flags; node != NULL; node = g_list_next (node)) {
			camel_folder_set_message_user_flag (folder, message_uid,
							    (char *) node->data, FALSE);
			g_free (node->data);
		}
		g_list_free (unset_user_flags);
		
		for (node = set_user_flags; node != NULL; node = g_list_next (node)) {
			camel_folder_set_message_user_flag (folder, message_uid,
							    (char *) node->data, TRUE);
			g_free (node->data);
		}
		g_list_free (set_user_flags);
		
		/* We don't wait for result */
		camel_folder_synchronize_message (folder, message_uid,
						  G_PRIORITY_DEFAULT_IDLE, NULL,
						  NULL, NULL);
	}

	if (_error)
		g_propagate_error (error, _error);

	return _error == NULL;
}

static void
im_mail_op_flag_message_thread (GSimpleAsyncResult *simple,
				GObject *object,
				GCancellable *cancellable)
{
	GError *_error = NULL;
	FlagMessageAsyncContext *context;

	context = (FlagMessageAsyncContext *)
		g_simple_async_result_get_op_res_gpointer (simple);

	im_mail_op_flag_message_sync (IM_SERVICE_MGR (object),
				      context->account_id,
				      context->folder_name,
				      context->message_uid,
				      context->set_flags,
				      context->unset_flags,
				      cancellable,
				      &_error);
	
	if (_error != NULL)
		g_simple_async_result_take_error (simple, _error);
}

/**
 * im_mail_op_flag_message_async:
 * @mgr: a #ImServiceMgr
 * @account_id: an account id
 * @folder_name: a folder name
 * @message_uid: a message uid
 * @set_flags: a string with the list of flags to set
 * @unset_flags: a string with the list of flags to unset
 * @io_priority: the I/O priority of the request
 * @cancellable: optional #GCancellable object, or %NULL,
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @userdata: data to pass to callback
 *
 * Asynchronously modifies the flags and user flags of the message with @message_uid 
 * from folder @folder_name in account @account_id.
 *
 * When the operation is finished, @callback is called. The you should call
 * im_mail_op_flag_message_finish() to get the result of the operation.
 */
void
im_mail_op_flag_message_async (ImServiceMgr *mgr,
			       const gchar *account_id,
			       const gchar *folder_name,
			       const gchar *message_uid,
			       const gchar *set_flags,
			       const gchar *unset_flags,
			       int io_priority,
			       GCancellable *cancellable,
			       GAsyncReadyCallback callback,
			       gpointer userdata)
{
	GSimpleAsyncResult *simple;
	FlagMessageAsyncContext *context;

	context = g_new0 (FlagMessageAsyncContext, 1);
	context->account_id = g_strdup (account_id);
	context->folder_name = g_strdup (folder_name);
	context->message_uid = g_strdup (message_uid);
	context->set_flags = g_strdup (set_flags);
	context->unset_flags = g_strdup (unset_flags);

	simple = g_simple_async_result_new (G_OBJECT (mgr),
					    callback, userdata,
					    im_mail_op_flag_message_async);

	g_simple_async_result_set_op_res_gpointer (simple, context, 
						   (GDestroyNotify) flag_message_async_context_free);

	g_simple_async_result_run_in_thread (simple,
					     im_mail_op_flag_message_thread,
					     io_priority, cancellable);
	g_object_unref (simple);
}

/**
 * im_mail_op_flag_message_finish:
 * @mgr: a #ImServiceMgr
 * @result: a #GAsyncResult
 * @error: (out) (allow-none): return location for a #GError, or %NULL
 *
 * Finishes the operation started with im_mail_op_flag_message_async().
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
im_mail_op_flag_message_finish (ImServiceMgr *mgr,
				GAsyncResult *result,
				GError **error)
{
	GSimpleAsyncResult *simple;

	g_return_val_if_fail (
		g_simple_async_result_is_valid (
		result, G_OBJECT (mgr), im_mail_op_flag_message_async), FALSE);

	simple = G_SIMPLE_ASYNC_RESULT (result);

	return !g_simple_async_result_propagate_error (simple, error);
}

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
	g_free (context);
}

/**
 * im_mail_op_composer_save_sync:
 * @folder: a #CamelFolder
 * @message: a #CamelMimeMessage
 * @body: a string with the plain text body
 * @attachment_uris: (element-type utf-8): list of attachment URIS
 * @uid: (out) (allow-none): the UID of the appended message.
 * @cancellable: optional #GCancellable object, or %NULL.
 * @error: (out) (allow-none): return location for a #GError, or %NULL.
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
