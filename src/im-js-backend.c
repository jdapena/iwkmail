/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-js-backend.c : exposes Iwkmail backend to JS */

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

#include "im-js-backend.h"

#include "im-account-mgr.h"
#include "im-account-mgr-helpers.h"
#include "im-enum-types.h"
#include "im-error.h"
#include "im-js-gobject-wrapper.h"
#include "im-js-utils.h"
#include "im-mail-ops.h"
#include "im-service-mgr.h"

#include <glib/gi18n.h>

typedef struct {
	JSGlobalContextRef context;
	JSObjectRef result_obj;
	JSValueRef result;
	gboolean has_result;
	GError *error;
	GCancellable *cancellable;
} ImJSCallContext;

static JSValueRef
on_call_context_cancel (JSContextRef ctx,
			JSObjectRef function,
			JSObjectRef thisObject,
			size_t argumentCount,
			const JSValueRef arguments[],
			JSValueRef *exception)
{
	ImJSCallContext *call_context;

	call_context = JSObjectGetPrivate (thisObject);
	if (call_context && call_context->cancellable) {
		g_cancellable_cancel (call_context->cancellable);
	}

	return JSValueMakeUndefined (ctx);
}

static ImJSCallContext *
im_js_call_context_new (JSContextRef context)
{
	ImJSCallContext *call_context;
	JSObjectRef call_context_cancel_obj;

	call_context = g_slice_new0 (ImJSCallContext);
	call_context->context = JSGlobalContextCreateInGroup (JSContextGetGroup (context), NULL);
	call_context->result_obj = JSObjectMake (context, NULL, NULL);
	JSObjectSetPrivate (call_context->result_obj, call_context);
	call_context_cancel_obj = JSObjectMakeFunctionWithCallback (context,
								   NULL,
								   on_call_context_cancel);
	im_js_object_set_property_from_value (context,
					      call_context->result_obj,
					      "cancel",
					      call_context_cancel_obj,
					      NULL);
	call_context->cancellable = g_cancellable_new ();
	JSValueProtect (call_context->context, call_context->result_obj);

	return call_context;
}

static void
im_js_call_context_dump_error (ImJSCallContext *call_context, JSValueRef *exception)
{
	GError *error = call_context->error;
	JSContextRef context = call_context->context;
	JSValueRef error_value;

	if (error == NULL)
		return;

	error_value = JSObjectMakeError (context, 0, 0, exception);

	if (*exception == NULL) {
		const char *code = NULL;
		if (error->domain == IM_ERROR_DOMAIN) {
			GEnumValue *value;
			value = g_enum_get_value (g_type_class_peek (IM_TYPE_ERROR_CODE), error->code);
			if (value)
				code = value->value_nick;
		} else {
			code = "internal-error";
		}
		im_js_object_set_property_from_string (context,
						       JSValueToObject (context,
									error_value,
									NULL),
						       "code", code,
						       exception);
	}

	if (*exception == NULL) {
		im_js_object_set_property_from_string (context,
						       JSValueToObject (context,
									error_value,
									NULL),
						       "message",
						       error->message,
						       exception);
	}
	
	if (*exception == NULL) {
		im_js_object_set_property_from_value (context,
						      call_context->result_obj,
						      "error",
						      error_value,
						      exception);
	}

	g_error_free (error);
	call_context->error = NULL;
}

static void
im_js_call_context_dump_result (ImJSCallContext *call_context, JSValueRef result)
{
	JSContextRef context = call_context->context;

	if (call_context->has_result && call_context->result != NULL)
		JSValueUnprotect (context, call_context->result);
	call_context->result = result;
	if (call_context->result != NULL)
		JSValueProtect (context, call_context->result);
	call_context->has_result = TRUE;
}

static gboolean
finish_im_js_call_context_idle (gpointer userdata)
{
	ImJSCallContext *call_context = (ImJSCallContext *) userdata;
	JSValueRef exception = NULL;
	JSValueRef callback;
	JSValueRef finish_callback;
	JSGlobalContextRef context = call_context->context;

	im_js_call_context_dump_error (call_context, &exception);

	if (exception == NULL) {
		callback = im_js_object_get_property (call_context->context,
						      call_context->result_obj,
						      call_context->error?"onError":"onSuccess",
						      &exception);
	}

	if (exception == NULL) {
		finish_callback = im_js_object_get_property (call_context->context,
							     call_context->result_obj,
							     "onFinish",
							     &exception);
	}

	if (exception == NULL && JSValueIsObject (context, callback)) {
		JSObjectRef callback_obj;
		size_t result_count;
		JSValueRef result_v[1];

		result_count = call_context->has_result && call_context->error == NULL;
		result_v[0] = call_context->result;
		callback_obj = JSValueToObject (context, callback, &exception);
		if (exception == NULL && JSObjectIsFunction (context, callback_obj)) {
			JSObjectCallAsFunction (context,
						callback_obj,
						call_context->result_obj,
						result_count, result_v, &exception);
		}
	}

	if (exception == NULL && JSValueIsObject (context, finish_callback)) {
		JSObjectRef finish_obj;

		finish_obj = JSValueToObject (context, finish_callback, &exception);
		if (exception == NULL && JSObjectIsFunction (context, finish_obj)) {
			JSObjectCallAsFunction (context,
						finish_obj,
						call_context->result_obj,
						0, NULL, &exception);
		}
	}

	JSValueUnprotect (context, call_context->result_obj);
	JSGlobalContextRelease (context);
	g_slice_free (ImJSCallContext, call_context);

	return FALSE;
}

static void
finish_im_js_call_context (ImJSCallContext *call_context)
{
	g_idle_add (finish_im_js_call_context_idle, call_context);
}

static JSValueRef
im_account_mgr_js_delete_account (JSContextRef context,
				  JSObjectRef function,
				  JSObjectRef this_object,
				  size_t argument_count,
				  const JSValueRef arguments[],
				  JSValueRef *exception)
{
	ImJSCallContext *call_context;
	JSStringRef account_id_str;
	char *account_id = NULL;

	call_context = im_js_call_context_new (context);

	if (argument_count != 1) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED,
			     _("Invalid arguments"));
		goto finish;
	}

	if (!JSValueIsString (context, arguments[0])) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED,
			     _("Account parameter is not a string"));
		goto finish;
	}

	account_id_str = JSValueToStringCopy (context, arguments[0], exception);
	if (account_id_str == NULL) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED,
			     _("Account parameter is not a string"));
		goto finish;
	}

	account_id = im_js_string_to_utf8 (account_id_str);
	JSStringRelease (account_id_str);

	im_account_mgr_remove_account (im_account_mgr_get_instance (),
				       account_id,
				       &call_context->error);

finish:
	g_free (account_id);
	finish_im_js_call_context (call_context);
	return call_context->result_obj;
}

static JSValueRef
im_account_mgr_js_get_accounts (JSContextRef context,
				JSObjectRef function,
				JSObjectRef this_object,
				size_t argument_count,
				const JSValueRef arguments[],
				JSValueRef *exception)
{
	ImJSCallContext *call_context;
	GSList *account_ids, *node;
	JSValueRef *args;
	size_t args_count;
	JSObjectRef array;
	int i;
	ImAccountMgr *account_mgr;

	call_context = im_js_call_context_new (context);

	if (argument_count != 0) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_ACCOUNT_MGR_GET_ACCOUNTS_FAILED,
			     _("Invalid arguments"));
		goto finish;
	}

	account_mgr = im_account_mgr_get_instance ();
	account_ids = im_account_mgr_get_account_ids (account_mgr, TRUE);
	args_count = g_slist_length (account_ids);
	args = g_new0 (JSValueRef, args_count);
	i = 0;
	for (node = account_ids; node != NULL; node = g_slist_next (node)) {
		  char *id = (char *) node->data;
		  ImAccountSettings *settings;

		  settings = im_account_mgr_load_account_settings (account_mgr, id);
		  args[i] = im_js_gobject_wrapper_wrap (im_js_gobject_wrapper_get_instance (),
							context,
							(GObject *) settings);

		  g_object_unref (settings);
		  i++;
	}
	array = JSObjectMakeArray (context, args_count,
				   (args_count > 0)?args:NULL,
				   exception);
	g_free (args);
	im_js_call_context_dump_result (call_context, array);

finish:
	finish_im_js_call_context (call_context);
	return call_context->result_obj;
}

static JSValueRef
im_account_mgr_js_add_account (JSContextRef context,
			       JSObjectRef function,
			       JSObjectRef this_object,
			       size_t argument_count,
			       const JSValueRef arguments[],
			       JSValueRef *exception)
{
	ImJSCallContext *call_context;
	ImAccountSettings *account;
	GError *_error = NULL;
	JSObjectRef js_object;

	call_context = im_js_call_context_new (context);

	if (argument_count != 1 ||
	    !JSValueIsObject (context, arguments[0])) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_ACCOUNT_MGR_GET_ACCOUNTS_FAILED,
			     _("Invalid arguments"));
		goto finish;
	}

	js_object = JSValueToObject (context, arguments[0], exception);
	account = IM_ACCOUNT_SETTINGS (im_js_gobject_wrapper_get_wrapped (im_js_gobject_wrapper_get_instance (),
									  js_object));

	if (!im_account_mgr_add_account_from_settings (im_account_mgr_get_instance (),
						       account)) {
		g_set_error (&_error, IM_ERROR_DOMAIN, IM_ERROR_ACCOUNT_MGR_ADD_ACCOUNT_FAILED,
			     _("Failed to create account"));
	}

	if (_error)
		g_propagate_error (&(call_context->error), _error);

finish:
	finish_im_js_call_context (call_context);
	return call_context->result_obj;
}

static const JSStaticFunction im_account_mgr_class_staticfuncs[] =
{
{ "addAccount", im_account_mgr_js_add_account, kJSPropertyAttributeNone },
{ "deleteAccount", im_account_mgr_js_delete_account, kJSPropertyAttributeNone },
{ "getAccounts", im_account_mgr_js_get_accounts, kJSPropertyAttributeNone },
{ NULL, NULL, 0 }
};

static const JSClassDefinition im_account_mgr_class_def =
{
0,
kJSClassAttributeNone,
"ImAccountMgrClass",
NULL,

NULL,
im_account_mgr_class_staticfuncs,

NULL,
NULL,

NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL
};

typedef struct {
	ImJSCallContext *call_context;
	char *newest_uid;
	char *oldest_uid;
	gint count;
} FetchMessagesContext;

static void
finish_fetch_messages (FetchMessagesContext *context)
{
	g_free (context->newest_uid);
	g_free (context->oldest_uid);
	finish_im_js_call_context (context->call_context);
	g_free (context);
}

static void
fetch_messages_refresh_info_cb (GObject *source_object,
				GAsyncResult *result,
				gpointer userdata)
{
	FetchMessagesContext *fm_context = (FetchMessagesContext *) userdata;
	ImJSCallContext *call_context = fm_context->call_context;
	JSContextRef context = call_context->context;
	GError *error = NULL;
	CamelFolder *folder = NULL;
	GPtrArray *uids;
	int i, j;

	im_mail_op_refresh_folder_info_finish (IM_SERVICE_MGR (source_object),
					       result,
					       &folder,
					       &error);

	if (folder) {
		GArray *new_messages_values, *messages_values;
		JSObjectRef new_messages_array, messages_array;
		JSObjectRef result;

		result = JSObjectMake (context, NULL, NULL);

		new_messages_values = g_array_new (TRUE, TRUE, sizeof(JSValueRef));
		messages_values = g_array_new (TRUE, TRUE, sizeof(JSValueRef));

		uids = camel_folder_get_uids (folder);
		camel_folder_sort_uids (folder, uids);
		
		/* Fetch first new messages */
		i = uids->len - 1;
		if (fm_context->newest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				CamelMessageInfo *mi;
				JSValueRef mi_value;
				if (g_strcmp0 (uid, fm_context->newest_uid) == 0)
					break;
				mi = camel_folder_get_message_info (folder, uid);
				mi_value = im_js_wrap_camel_message_info (context, mi);
				g_array_append_val (new_messages_values, mi_value);
				camel_folder_free_message_info (folder, mi);
				i--;
			}
		}
		if (fm_context->oldest_uid != NULL) {
			while (i >= 0) {
				const char *uid = uids->pdata[i];
				if (g_strcmp0 (uid, fm_context->oldest_uid) == 0)
					break;
				i--;
			}
			i--;
		}
		for (j = 0; j < fm_context->count; j++) {
			const char *uid;
			CamelMessageInfo *mi;
			JSValueRef mi_value;
			if (i < 0)
				break;
			uid = (const char *) uids->pdata[i];
			mi = camel_folder_get_message_info (folder, uid);
			mi_value = im_js_wrap_camel_message_info (context, mi);
			g_array_append_val (messages_values, mi_value);
			camel_folder_free_message_info (folder, mi);
			i--;
		}
		camel_folder_free_uids (folder, uids);

		new_messages_array = JSObjectMakeArray (context,
							new_messages_values->len,
							(JSValueRef *) new_messages_values->data,
							NULL);
		messages_array = JSObjectMakeArray (context,
						    messages_values->len,
						    (JSValueRef *) messages_values->data,
						    NULL);
		g_array_free (new_messages_values, TRUE);
		g_array_free (messages_values, TRUE);

		im_js_object_set_property_from_value (context, result,
						      "new_messages", new_messages_array, NULL);
		im_js_object_set_property_from_value (context, result,
						      "messages", messages_array, NULL);
		im_js_call_context_dump_result (call_context, result);
	}

	/* Unless we got a cancel, we ignore the error */
	if (error && 
	    !(error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)) {
		g_clear_error (&error);
	}

	if (error)
		g_propagate_error (&(fm_context->call_context->error), error);

	finish_fetch_messages (fm_context);
}

static JSValueRef
im_service_mgr_js_fetch_messages (JSContextRef context,
				  JSObjectRef function,
				  JSObjectRef this_object,
				  size_t argument_count,
				  const JSValueRef arguments[],
				  JSValueRef *exception)
{
	GError *_error = NULL;
	char *account_id = NULL, *folder_name = NULL;
	JSValueRef _exception = NULL;
	FetchMessagesContext *fm_context = g_new0 (FetchMessagesContext, 1);
	ImJSCallContext *call_context = im_js_call_context_new (context);

	fm_context->call_context = call_context;

	if (argument_count != 5 ||
	    !JSValueIsString (context, arguments[0]) ||
	    !JSValueIsString (context, arguments[1]) ||
	    !JSValueIsNumber (context, arguments[2]) ||
	    (!JSValueIsString (context, arguments[3]) && !JSValueIsNull (context, arguments[3])) ||
	    (!JSValueIsString (context, arguments[4]) && !JSValueIsNull (context, arguments[4]))) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_SERVICE_MGR_FETCH_MESSAGES_FAILED,
			     _("Invalid arguments"));
		goto finish;
	}

	if (_exception == NULL)
		account_id = im_js_value_to_utf8 (context, arguments[0], &_exception);
	if (_exception == NULL)
		folder_name = im_js_value_to_utf8 (context, arguments[1], &_exception);
	if (_exception == NULL)
		fm_context->count = (int) JSValueToNumber (context, arguments[2], &_exception);
	if (_exception == NULL)
		fm_context->newest_uid = im_js_value_to_utf8 (context, arguments[3], &_exception);
	if (_exception == NULL)
		fm_context->oldest_uid = im_js_value_to_utf8 (context, arguments[4], &_exception);

	if (_error)
		g_propagate_error (&(call_context->error), _error);

	if (_exception == NULL)
		im_mail_op_refresh_folder_info_async (im_service_mgr_get_instance (),
						      account_id,
						      folder_name,
						      G_PRIORITY_DEFAULT_IDLE,
						      call_context->cancellable,
						      fetch_messages_refresh_info_cb,
						      fm_context);
	else
		finish_fetch_messages (fm_context);

finish:
	g_free (account_id);
	g_free (folder_name);
	return call_context->result_obj;
}

static void
flag_message_mail_op_cb (GObject *object,
			 GAsyncResult *result,
			 gpointer userdata)
{
	GError *_error = NULL;
	ImJSCallContext *call_context = (ImJSCallContext *) userdata;

	im_mail_op_flag_message_finish (IM_SERVICE_MGR (object),
					result, &_error);
	if (_error)
		g_propagate_error (&(call_context->error), _error);
	finish_im_js_call_context (call_context);
}

static JSValueRef
im_service_mgr_js_flag_message (JSContextRef context,
				JSObjectRef function,
				JSObjectRef this_object,
				size_t argument_count,
				const JSValueRef arguments[],
				JSValueRef *exception)
{
	ImJSCallContext *call_context;
	GError *_error = NULL;
	char *account_id, *folder_name, *message_uid;
	char *set_flags, *unset_flags;
	JSValueRef _exception = NULL;

	call_context = im_js_call_context_new (context);

	if (argument_count != 5 ||
	    !JSValueIsString (context, arguments[0]) ||
	    !JSValueIsString (context, arguments[1]) ||
	    !JSValueIsString (context, arguments[2]) ||
	    !JSValueIsString (context, arguments[3]) ||
	    !JSValueIsString (context, arguments[4])) {
		g_set_error (&(call_context->error),
			     IM_ERROR_DOMAIN,
			     IM_ERROR_SERVICE_MGR_FLAG_MESSAGE_FAILED,
			     _("Invalid arguments"));
		goto finish;
	}

	if (_exception == NULL)
		account_id = im_js_value_to_utf8 (context, arguments[0], &_exception);
	if (_exception == NULL)
		folder_name = im_js_value_to_utf8 (context, arguments[1], &_exception);
	if (_exception == NULL)
		message_uid = im_js_value_to_utf8 (context, arguments[2], &_exception);
	if (_exception == NULL)
		set_flags = im_js_value_to_utf8 (context, arguments[3], &_exception);
	if (_exception == NULL)
		unset_flags = im_js_value_to_utf8 (context, arguments[4], &_exception);

	if (_error)
		g_propagate_error (&(call_context->error), _error);

	if (_exception == NULL)
		im_mail_op_flag_message_async (im_service_mgr_get_instance (),
					       account_id, folder_name, message_uid,
					       set_flags, unset_flags,
					       G_PRIORITY_DEFAULT_IDLE,
					       call_context->cancellable,
					       flag_message_mail_op_cb,
					       call_context);
	else
		finish_im_js_call_context (call_context);

finish:
	return call_context->result_obj;
}



static const JSStaticFunction im_service_mgr_class_staticfuncs[] =
{
{ "flagMessage", im_service_mgr_js_flag_message, kJSPropertyAttributeNone },
{ "fetchMessages", im_service_mgr_js_fetch_messages, kJSPropertyAttributeNone },
{ NULL, NULL, 0 }
};

static const JSClassDefinition im_service_mgr_class_def =
{
0,
kJSClassAttributeNone,
"ImServiceMgrClass",
NULL,

NULL,
im_service_mgr_class_staticfuncs,

NULL,
NULL,

NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL
};


static void
im_account_mgr_setup_js_class (JSGlobalContextRef context)
{
	JSClassRef account_mgr_class;
	JSObjectRef account_mgr_obj;
	JSClassRef service_mgr_class;
	JSObjectRef service_mgr_obj;
	JSObjectRef iwk_obj;
	JSObjectRef global_obj;

	account_mgr_class = JSClassCreate (&im_account_mgr_class_def);
	account_mgr_obj = JSObjectMake (context, account_mgr_class, NULL);

	service_mgr_class = JSClassCreate (&im_service_mgr_class_def);
	service_mgr_obj = JSObjectMake (context, service_mgr_class, NULL);

	iwk_obj = JSObjectMake (context, NULL, NULL);
	im_js_object_set_property_from_value (context, iwk_obj, "AccountMgr",
					      account_mgr_obj, NULL);
	im_js_object_set_property_from_value (context, iwk_obj, "ServiceMgr",
					      service_mgr_obj, NULL);
	im_js_object_set_property_from_value (context, iwk_obj, "AccountSettings",
					      (JSValueRef) im_js_gobject_wrapper_init_constructor (im_js_gobject_wrapper_get_instance (),
										      context,
										      IM_TYPE_ACCOUNT_SETTINGS),
					      NULL);
	global_obj = JSContextGetGlobalObject (context);
	im_js_object_set_property_from_value (context, global_obj, "iwk",
					      iwk_obj, NULL);

}


void im_js_backend_setup_context (JSGlobalContextRef context)
{
	im_account_mgr_setup_js_class (context);
}
