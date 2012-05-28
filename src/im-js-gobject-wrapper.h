/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-js-gobject-wrapper.h : factory to wrap GObjects to a JSC context */

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


#ifndef __IM_JS_GOBJECT_WRAPPER_H__
#define __IM_JS_GOBJECT_WRAPPER_H__

#include <glib-object.h>
#include <JavaScriptCore/JavaScript.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_JS_GOBJECT_WRAPPER             (im_js_gobject_wrapper_get_type())
#define IM_JS_GOBJECT_WRAPPER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_JS_GOBJECT_WRAPPER,ImJSGObjectWrapper))
#define IM_JS_GOBJECT_WRAPPER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_JS_GOBJECT_WRAPPER,ImJSGObjectWrapperClass))
#define IM_IS_JS_GOBJECT_WRAPPER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_JS_GOBJECT_WRAPPER))
#define IM_IS_JS_GOBJECT_WRAPPER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_JS_GOBJECT_WRAPPER))
#define IM_JS_GOBJECT_WRAPPER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_JS_GOBJECT_WRAPPER,ImJSGObjectWrapperClass))

typedef struct _ImJSGObjectWrapper      ImJSGObjectWrapper;
typedef struct _ImJSGObjectWrapperClass ImJSGObjectWrapperClass;

struct _ImJSGObjectWrapper {
	GObject parent;
};

struct _ImJSGObjectWrapperClass {
	GObjectClass parent_class;
};

GType  im_js_gobject_wrapper_get_type   (void) G_GNUC_CONST;

ImJSGObjectWrapper*    im_js_gobject_wrapper_get_instance (void);

JSValueRef im_js_gobject_wrapper_wrap (ImJSGObjectWrapper *wrapper,
				       JSContextRef context,
				       GObject *obj);
JSObjectRef im_js_gobject_wrapper_init_constructor (ImJSGObjectWrapper *wrapper,
						    JSContextRef context,
						    GType constructor_type);

GObject *im_js_gobject_wrapper_get_wrapped (ImJSGObjectWrapper *wrapper,
					    JSObjectRef js_object);

G_END_DECLS

#endif /* __IM_JS_GOBJECT_WRAPPER_H__ */
