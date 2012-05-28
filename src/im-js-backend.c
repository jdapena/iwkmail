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

#include <glib/gi18n.h>

typedef struct {
	JSGlobalContextRef context;
	JSObjectRef result_obj;
	JSValueRef result;
	gboolean has_result;
	GError *error;
} ImJSCallContext;

static ImJSCallContext *
im_js_call_context_new (JSContextRef context)
{
	ImJSCallContext *call_context;

	call_context = g_slice_new0 (ImJSCallContext);
	call_context->context = JSGlobalContextCreateInGroup (JSContextGetGroup (context), NULL);
	call_context->result_obj = JSObjectMake (context, NULL, NULL);
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
	JSGlobalContextRef context = call_context->context;

	im_js_call_context_dump_error (call_context, &exception);

	if (exception == NULL) {
		callback = im_js_object_get_property (call_context->context,
						      call_context->result_obj,
						      call_context->error?"onError":"onSuccess",
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

static const JSStaticFunction im_account_mgr_class_staticfuncs[] =
{
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


static void
im_account_mgr_setup_js_class (JSGlobalContextRef context)
{
	JSClassRef account_mgr_class;
	JSObjectRef account_mgr_obj;
	JSObjectRef global_obj;

	account_mgr_class = JSClassCreate (&im_account_mgr_class_def);
	account_mgr_obj = JSObjectMake (context, account_mgr_class, NULL);

	global_obj = JSContextGetGlobalObject (context);
	im_js_object_set_property_from_value (context, global_obj, "imAccountMgr",
					      account_mgr_obj, NULL);
}


void im_js_backend_setup_context (JSGlobalContextRef context)
{
	im_account_mgr_setup_js_class (context);
}
