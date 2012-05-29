/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-js-utils.c : helpers between JSC and Glib */

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

#include "im-js-utils.h"

#include <glib/gi18n.h>

char *
im_js_string_to_utf8 (JSStringRef js_string)
{
  int length;
  char *result;

  length = JSStringGetMaximumUTF8CStringSize (js_string);
  if (length == 0)
    return NULL;
  result = g_malloc0 (length);
  JSStringGetUTF8CString (js_string, result, length);
  return result;
}

char *
im_js_value_to_utf8 (JSContextRef context,
		     JSValueRef js_value,
		     JSValueRef *exception)
{
	JSStringRef string;
	JSValueRef _exception = NULL;
	char *result = NULL;

	if (JSValueIsNull (context, js_value))
		return NULL;

	string = JSValueToStringCopy (context, js_value, &_exception);
	if (_exception == NULL) {
		result = im_js_string_to_utf8 (string);
		JSStringRelease (string);
	}

	if (exception)
		*exception = _exception;

	return result;
}

JSValueRef
im_js_object_get_property (JSContextRef context,
			   JSObjectRef obj,
			   const char *name,
			   JSValueRef *exception)
{
  JSStringRef name_string;
  JSValueRef result;

  name_string = JSStringCreateWithUTF8CString (name);
  result = JSObjectGetProperty (context, obj, name_string, exception);
  JSStringRelease (name_string);

  return result;
}

void
im_js_object_set_property_from_string (JSContextRef context,
				       JSObjectRef obj,
				       const char *name,
				       const char *value,
				       JSValueRef *exception)
{
  JSStringRef name_string;
  JSStringRef value_string;
  
  name_string = JSStringCreateWithUTF8CString (name);
  value_string = JSStringCreateWithUTF8CString (value);
  JSObjectSetProperty (context, obj, 
                       name_string, JSValueMakeString (context, value_string),
                       kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete,
                       exception);
  JSStringRelease (name_string);
  JSStringRelease (value_string);
}

void
im_js_object_set_property_from_value (JSContextRef context,
				      JSObjectRef obj,
				      const char *name,
				      JSValueRef value,
				      JSValueRef *exception)
{
  JSStringRef name_string;

  name_string = JSStringCreateWithUTF8CString (name);
  JSObjectSetProperty (context, obj, 
                       name_string, value,
                       kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete,
                       exception);
  JSStringRelease (name_string);
}

