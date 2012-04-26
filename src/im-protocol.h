/* Based on ModestProtocol */

/* Copyright (c) 2008, Nokia Corporation
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


/* im-account-settings.h */

#ifndef __IM_PROTOCOL_H__
#define __IM_PROTOCOL_H__

#include <glib-object.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_PROTOCOL             (im_protocol_get_type())
#define IM_PROTOCOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_PROTOCOL,ImProtocol))
#define IM_PROTOCOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_PROTOCOL,ImProtocolClass))
#define IM_IS_PROTOCOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_PROTOCOL))
#define IM_IS_PROTOCOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_PROTOCOL))
#define IM_PROTOCOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_PROTOCOL,ImProtocolClass))

#define IM_PROTOCOL_TYPE_INVALID -1

typedef gchar * (*TranslationFunc) (gpointer userdata, va_list args);

typedef struct _ImProtocol      ImProtocol;
typedef struct _ImProtocolClass ImProtocolClass;

typedef guint ImProtocolType;

struct _ImProtocol {
	GObject parent;
};

struct _ImProtocolClass {
	GObjectClass parent_class;
};

/**
 * im_protocol_get_type:
 *
 * Returns: GType of the account store
 */
GType  im_protocol_get_type   (void) G_GNUC_CONST;

/**
 * im_protocol_new:
 *
 * creates a new instance of #ImProtocol
 *
 * Returns: a #ImProtocol
 */
ImProtocol*    im_protocol_new (const gchar *name,
				const gchar *display_name);

/**
 * im_protocol_get_name:
 * @self: a #ImProtocol
 *
 * get the protocol unique name (used for storing conf and identifying the protocol with a string)
 *
 * Returns: a string
 */
const gchar* im_protocol_get_name (ImProtocol *self);

/**
 * im_protocol_set_name:
 * @self: a #ImProtocol
 * @name: the protocol unique name.
 *
 * set @name as the protocol unique name .
 */
void         im_protocol_set_name (ImProtocol *self,
				   const gchar *name);
/**
 * im_protocol_get_display_name:
 * @self: a #ImProtocol
 *
 * get the display name for the protocol
 *
 * Returns: a string
 */
const gchar* im_protocol_get_display_name (ImProtocol *self);

/**
 * im_protocol_set_display_name:
 * @settings: a #ImProtocol
 * @display_name: a string.
 *
 * set @display_name of the account.
 */
void         im_protocol_set_display_name (ImProtocol *protocol,
					   const gchar *display_name);
/**
 * im_protocol_get_type_id:
 * @self: a #ImProtocol
 *
 * get the protocol type id.
 *
 * Returns: a #ImProtocolType
 */
ImProtocolType im_protocol_get_type_id (ImProtocol *self);

/**
 * im_protocol_get:
 * @protocol: a #ImProtocol
 * @key: a string
 *
 * obtains the value of @key for @protocol
 *
 * Returns: a string
 */
const gchar *
im_protocol_get (ImProtocol *protocol,
		 const gchar *key);

/**
 * im_protocol_set:
 * @protocol: a #ImProtocol
 * @key: a string
 * @value: a string
 *
 * sets @value as the value for @key in @protocol
 */
void
im_protocol_set (ImProtocol *protocol,
		 const gchar *key, const gchar *value);

/**
 * im_protocol_set_translation:
 * @protocol: a #ImProtocol
 * @id: the id for the translation set
 * @translation_func: the function used to obtain the translation
 *
 * sets @translation_func as the way to compose the translation for @id
 */
void
im_protocol_set_translation (ImProtocol *protocol,
			     const gchar *id,
			     TranslationFunc translation_func,
			     gpointer userdata,
			     GDestroyNotify data_destroy_func);

/**
 * im_protocol_get_translation:
 * @protocol: a @ImProtocol
 * @id: the id for the translation set
 * @...: the parameters for the translation (pritntf style)
 *
 * applies the translation with parameters to obtain the full string expected.
 *
 * Returns: a newly allocated string
 */
gchar *
im_protocol_get_translation (ImProtocol *protocol,
			     const gchar *id,
			     ...);

/**
 * im_protocol_va_get_translation:
 * @protocol: a @ImProtocol
 * @id: the id for the translation set
 * @args: a @va_list of the parameters for the translation
 *
 * applies the translation with parameters to obtain the full string expected.
 *
 * Returns: a newly allocated string
 */
gchar *
im_protocol_va_get_translation (ImProtocol *protocol,
				const gchar *id,
				va_list args);

G_END_DECLS

#endif /* __IM_PROTOCOL_H__ */
