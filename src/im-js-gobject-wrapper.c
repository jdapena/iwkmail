/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-js-gobject-wrapper.c : factory to wrap GObjects to a JSC context */

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

#include <im-js-gobject-wrapper.h>

#include <im-js-utils.h>

#include <glib/gi18n.h>

typedef struct _ImJSGObjectWrapperPrivate ImJSGObjectWrapperPrivate;
struct _ImJSGObjectWrapperPrivate {
	GHashTable *class_hash;
};

/* 'private'/'protected' functions */
static void   im_js_gobject_wrapper_class_init (ImJSGObjectWrapperClass *klass);
static void   im_js_gobject_wrapper_finalize   (GObject *obj);
static void   im_js_gobject_wrapper_instance_init (ImJSGObjectWrapper *obj);

#define GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
							 IM_TYPE_JS_GOBJECT_WRAPPER, \
							 ImJSGObjectWrapperPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
im_js_gobject_wrapper_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ImJSGObjectWrapperClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) im_js_gobject_wrapper_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ImJSGObjectWrapper),
			0,      /* n_preallocs */
			(GInstanceInitFunc) im_js_gobject_wrapper_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ImJSGObjectWrapper",
						  &my_info, 0);
	}
	return my_type;
}

static void
im_js_gobject_wrapper_class_init (ImJSGObjectWrapperClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = im_js_gobject_wrapper_finalize;

	g_type_class_add_private (object_class,
				  sizeof(ImJSGObjectWrapperPrivate));
}

static void
im_js_gobject_wrapper_instance_init (ImJSGObjectWrapper *obj)
{
	ImJSGObjectWrapperPrivate *priv = GET_PRIVATE (obj);

	priv->class_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void   
im_js_gobject_wrapper_finalize   (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static ImJSGObjectWrapper*
im_js_gobject_wrapper_new ()
{
	return g_object_new (IM_TYPE_JS_GOBJECT_WRAPPER, NULL);
}

ImJSGObjectWrapper *
im_js_gobject_wrapper_get_instance ()
{
	static ImJSGObjectWrapper *singleton = 0;

	if (singleton == 0)
		singleton = im_js_gobject_wrapper_new ();

	return singleton;
}

static JSValueRef
gvalue_to_js_value (JSContextRef context,
		    GValue *value,
		    GType value_type,
		    JSValueRef *exception)
{
	ImJSGObjectWrapper *wrapper;
	JSValueRef result;

	wrapper = im_js_gobject_wrapper_get_instance ();

	if (value_type == G_TYPE_BOOLEAN) {
		gboolean b = g_value_get_boolean (value);
		result = JSValueMakeBoolean (context, b);
	} else if (value_type == G_TYPE_CHAR) {
		gint8 c = g_value_get_schar (value);
		result = JSValueMakeNumber (context, (double) c);
	} else if (value_type == G_TYPE_UCHAR) {
		guchar c = g_value_get_uchar (value);
		result = JSValueMakeNumber (context, (double) c);
	} else if (value_type == G_TYPE_INT) {
		gint i = g_value_get_int (value);
		result = JSValueMakeNumber (context, (double) i);
	} else if (value_type == G_TYPE_UINT) {
		guint i = g_value_get_uint (value);
		result = JSValueMakeNumber (context, (double) i);
	} else if (value_type == G_TYPE_LONG) {
		glong l = g_value_get_long (value);
		result = JSValueMakeNumber (context, (double) l);
	} else if (value_type == G_TYPE_ULONG) {
		gulong l = g_value_get_long (value);
		result = JSValueMakeNumber (context, (double) l);
	} else if (value_type == G_TYPE_INT64) {
		gint64 i = g_value_get_int64 (value);
		result = JSValueMakeNumber (context, (double) i);
	} else if (value_type == G_TYPE_UINT64) {
		guint64 i = g_value_get_uint64 (value);
		result = JSValueMakeNumber (context, (double) i);
	} else if (value_type == G_TYPE_FLOAT) {
		gfloat f = g_value_get_float (value);
		result = JSValueMakeNumber (context, (double) f);
	} else if (value_type == G_TYPE_DOUBLE) {
		gdouble f = g_value_get_double (value);
		result = JSValueMakeNumber (context, (double) f);
	} else if (G_TYPE_IS_ENUM (value_type)) {
		gint v = g_value_get_enum (value);
		GEnumClass *enum_class = g_type_class_peek (value_type);
		GEnumValue *enum_value = g_enum_get_value (enum_class, v);
		JSStringRef string = JSStringCreateWithUTF8CString (enum_value->value_nick);
		result = JSValueMakeString (context, string);
		JSStringRelease (string);
	} else if (G_TYPE_IS_FLAGS (value_type)) {
		guint v = g_value_get_flags (value);
		result = JSValueMakeNumber (context, (double) v);
	} else if (value_type == G_TYPE_STRING) {
		const gchar *s = g_value_get_string (value);
		JSStringRef string = JSStringCreateWithUTF8CString (s);
		result = JSValueMakeString (context, string);
		JSStringRelease (string);
	} else if (G_TYPE_IS_OBJECT (value_type)) {
		GObject *o = g_value_get_object (value);
		result = im_js_gobject_wrapper_wrap (wrapper, context, o);
	} else {
		g_warning (_("%s: type %s not supported"), __FUNCTION__,
			   g_type_name (value_type));
		result = JSValueMakeNull (context);
	}

	return result;
}

static void
js_value_to_boolean (JSContextRef context,
		     JSValueRef js_value,
		     GValue *value,
		     JSValueRef *exception)
{
	switch (JSValueGetType (context, js_value)) {
	case kJSTypeUndefined:
	case kJSTypeNull:
		g_value_set_boolean (value, FALSE);
		break;
	case kJSTypeBoolean:
		g_value_set_boolean (value, JSValueToBoolean (context, js_value));
		break;
	case kJSTypeNumber:
		g_value_set_boolean (value, JSValueToNumber (context, js_value, exception) != 0);
		break;
	case kJSTypeString:
		{
			JSStringRef string;
			gchar *utf8;
			string = JSValueToStringCopy (context, js_value, exception);
			utf8 = im_js_string_to_utf8 (string);
			g_value_set_boolean (value, (utf8 != NULL && 
						     g_strcmp0 (utf8, "0") != 0));
			g_free (utf8);
			JSStringRelease (string);
		}
		break;
	case kJSTypeObject:
		g_value_set_boolean (value, TRUE);
	}
}

static void
js_value_to_char (JSContextRef context,
		  JSValueRef js_value,
		  GValue *value,
		  GType value_type,
		  JSValueRef *exception)
{
	gunichar c;
	gchar str[6];

	switch (JSValueGetType (context, js_value)) {
	case kJSTypeUndefined:
	case kJSTypeNull:
		c = 0;
		break;
	case kJSTypeBoolean:
		c = JSValueToBoolean (context, js_value);
		break;
	case kJSTypeNumber:
		c = JSValueToNumber (context, js_value, exception);
		break;
	case kJSTypeString:
		{
			JSStringRef string;
			gchar *utf8;
			c = 0;
			string = JSValueToStringCopy (context, js_value, exception);
			utf8 = im_js_string_to_utf8 (string);
			if (utf8)
				c = g_utf8_get_char (utf8);
			g_free (utf8);
			JSStringRelease (string);
		}
		break;
	case kJSTypeObject:
		c = 0;
	}
	g_unichar_to_utf8 (c, str);
	if (value_type == G_TYPE_CHAR)
		g_value_set_schar (value, (gint8) str[0]);
	else if (value_type == G_TYPE_UCHAR)
		g_value_set_uchar (value, (guchar) str[0]);
}

static void
js_value_to_number (JSContextRef context,
		    JSValueRef js_value,
		    GValue *value,
		    GType value_type,
		    JSValueRef *exception)
{
	gdouble v;

	switch (JSValueGetType (context, js_value)) {
	case kJSTypeUndefined:
	case kJSTypeNull:
		v = 0;
		break;
	case kJSTypeBoolean:
		v = JSValueToBoolean (context, js_value)?1:0;
		break;
	case kJSTypeNumber:
		v = JSValueToNumber (context, js_value, exception);
		break;
	case kJSTypeString:
		{
			JSStringRef string;
			gchar *utf8;
			v = 0;
			string = JSValueToStringCopy (context, js_value, exception);
			utf8 = im_js_string_to_utf8 (string);
			if (utf8)
				v = g_ascii_strtod (utf8, NULL);
			g_free (utf8);
			JSStringRelease (string);
		}
		break;
	case kJSTypeObject:
		v = 0;
	}
	if (value_type == G_TYPE_INT) {
		g_value_set_int (value, (gint) v);
	} else if (value_type == G_TYPE_UINT) {
		g_value_set_uint (value, (guint) v);
	} else if (value_type == G_TYPE_LONG) {
		g_value_set_long (value, (glong) v);
	} else if (value_type == G_TYPE_ULONG) {
		g_value_set_ulong (value, (gulong) v);
	} else if (value_type == G_TYPE_INT64) {
		g_value_set_int64 (value, (gint64) v);
	} else if (value_type == G_TYPE_UINT64) {
		g_value_set_uint64 (value, (guint64) v);
	} else if (value_type == G_TYPE_FLOAT) {
		g_value_set_float (value, (gfloat) v);
	} else if (value_type == G_TYPE_DOUBLE) {
		g_value_set_double (value, v);
	}
}

static void
js_value_to_gvalue (JSContextRef context,
		    JSValueRef js_value,
		    GValue *value,
		    GType value_type,
		    JSValueRef *exception)
{
	if (value_type == G_TYPE_BOOLEAN) {
		js_value_to_boolean (context, js_value, value, exception);
	} else if (value_type == G_TYPE_CHAR ||
		   value_type == G_TYPE_UCHAR) {
		js_value_to_char (context, js_value, value, value_type, exception);
	} else if (value_type == G_TYPE_INT ||
		   value_type == G_TYPE_UINT ||
		   value_type == G_TYPE_LONG ||
		   value_type == G_TYPE_ULONG ||
		   value_type == G_TYPE_UINT64 ||
		   value_type == G_TYPE_FLOAT ||
		   value_type == G_TYPE_DOUBLE) {
		js_value_to_number (context, js_value, value, value_type, exception);
	} else if (G_TYPE_IS_OBJECT (value_type)) {
		GObject *obj = NULL;

		if (JSValueIsObject (context, js_value)) {
			JSObjectRef js_o;

			js_o = JSValueToObject (context, js_value, exception);
			obj = (GObject *) JSObjectGetPrivate (js_o);
		}
		g_value_set_object (value, (obj && G_IS_OBJECT (obj))?obj:0);
	} else {
		g_warning (_("%s: type %s not supported"), __FUNCTION__,
			   g_type_name (value_type));
	}
}


static void
wrapper_initialize (JSContextRef context, JSObjectRef jsobj)
{
}

static void
wrapper_finalize (JSObjectRef jsobj)
{
	GObject *obj;

	obj = (GObject *) JSObjectGetPrivate (jsobj);

	if (obj != NULL) {
		g_object_unref (obj);
		JSObjectSetPrivate (jsobj, NULL);
	}
}

static bool
wrapper_has_property (JSContextRef context, 
		      JSObjectRef js_object,
		      JSStringRef property_name)
{
	GObject *obj;
	char *property_utf8;
	bool result;

	obj = (GObject *) JSObjectGetPrivate (js_object);
	property_utf8 = im_js_string_to_utf8 (property_name);
	g_strdelimit (property_utf8, "_", '-');

	result = g_object_class_find_property (G_OBJECT_GET_CLASS (obj),
					       property_utf8) != NULL;

	g_free (property_utf8);

	return result;
}

static void
wrapper_get_property_names (JSContextRef context, 
			    JSObjectRef js_object,
			    JSPropertyNameAccumulatorRef property_names)
{
	GObject *obj;
	GParamSpec **param_specs;
	guint n_properties, i;

	obj = (GObject *) JSObjectGetPrivate (js_object);
	param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
						      &n_properties);

	for (i = 0; i < n_properties; i++) {
		JSStringRef string;
		gchar *js_property_name;

		js_property_name = g_strdup (g_param_spec_get_name (param_specs[i]));
		g_strdelimit (js_property_name, "-", '_');
		string = JSStringCreateWithUTF8CString (js_property_name);
		g_free (js_property_name);
		JSPropertyNameAccumulatorAddName (property_names,
						  string);
		JSStringRelease (string);
	}
	g_free (param_specs);
}

static JSValueRef
wrapper_get_property (JSContextRef context,
		      JSObjectRef js_object,
		      JSStringRef property_name,
		      JSValueRef *exception)
{
	GObject *obj;
	char *property_utf8;
	JSValueRef result;
	GParamSpec *param_spec;
	JSValueRef _exception = NULL;
	GValue value = G_VALUE_INIT;

	obj = (GObject *) JSObjectGetPrivate (js_object);
	property_utf8 = im_js_string_to_utf8 (property_name);
	g_strdelimit (property_utf8, "_", '-');

	param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj),
						   property_utf8);
	if (param_spec == NULL) {
		_exception = JSObjectMakeError (context, 0, NULL, &_exception);
	}
	if (_exception == NULL) {
		GType value_type = G_PARAM_SPEC_VALUE_TYPE (param_spec);
		g_value_init (&value, value_type);
		g_object_get_property (obj, property_utf8, &value);

		result = gvalue_to_js_value (context, &value, value_type, &_exception);
	}

	g_free (property_utf8);
	
	if (exception)
		*exception = _exception;

	return result;
}

static bool
wrapper_set_property (JSContextRef context,
		      JSObjectRef js_object,
		      JSStringRef property_name,
		      JSValueRef js_value,
		      JSValueRef *exception)
{
	GObject *obj;
	char *property_utf8;
	GParamSpec *param_spec;
	JSValueRef _exception = NULL;
	GValue value;

	obj = (GObject *) JSObjectGetPrivate (js_object);
	property_utf8 = im_js_string_to_utf8 (property_name);
	g_strdelimit (property_utf8, "_", '-');

	param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj),
						   property_utf8);
	if (param_spec == NULL) {
		_exception = JSObjectMakeError (context, 0, NULL, &_exception);
	}
	if (_exception == NULL) {
		GType value_type = G_PARAM_SPEC_VALUE_TYPE (param_spec);
		g_value_init (&value, value_type);
		js_value_to_gvalue (context, js_value, &value, value_type, &_exception);
		g_object_set_property (obj, property_utf8, &value);
	}

	g_free (property_utf8);
	
	if (exception)
		*exception = _exception;

	return _exception != NULL;
}

static JSClassRef
get_js_class (ImJSGObjectWrapper *wrapper,
	      GType obj_type)
{
	ImJSGObjectWrapperPrivate *priv = GET_PRIVATE (wrapper);
	JSClassRef js_class;
	
	js_class = g_hash_table_lookup (priv->class_hash, (gpointer) obj_type);

	if (js_class == NULL) {
		JSClassRef parent_js_class = NULL;
		JSClassDefinition definition = {0,};
		if (obj_type != G_TYPE_OBJECT) {
			GType parent_type = G_TYPE_INVALID;
			parent_type = g_type_parent (obj_type);
			parent_js_class = get_js_class (wrapper, parent_type);
		}

		definition.version = 0;
		definition.className = g_type_name (obj_type);
		definition.attributes = kJSClassAttributeNone;
		definition.parentClass = parent_js_class;
		definition.staticValues = NULL;
		definition.staticFunctions = NULL;
		definition.initialize = wrapper_initialize;
		definition.finalize = wrapper_finalize;
		definition.hasProperty = wrapper_has_property;
		definition.getProperty = wrapper_get_property;
		definition.setProperty = wrapper_set_property;
		definition.getPropertyNames = wrapper_get_property_names;

		js_class = JSClassCreate (&definition);
	}

	return js_class;
}

JSValueRef
im_js_gobject_wrapper_wrap (ImJSGObjectWrapper *wrapper,
			    JSContextRef context,
			    GObject *obj)
{
	GType obj_type;
	JSClassRef js_class;
	JSValueRef result;
	
	obj_type = G_OBJECT_TYPE (obj);
	js_class = get_js_class (wrapper, obj_type);

	result = JSObjectMake (context, js_class, g_object_ref (obj));

	return result;
}
