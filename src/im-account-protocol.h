/* Based on ModestAccountProtocol */

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

#ifndef __IM_ACCOUNT_PROTOCOL_H__
#define __IM_ACCOUNT_PROTOCOL_H__

#include "im-protocol.h"
#include "im-pair.h"

#define IM_ACCOUNT_AUTH_MECH	 "auth_mech"	     /* string */
#define IM_ACCOUNT_AUTH_MECH_VALUE_NONE "none"
#define IM_ACCOUNT_AUTH_MECH_VALUE_LOGIN "password"
#define IM_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5 "cram-md5"


G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_ACCOUNT_PROTOCOL             (im_account_protocol_get_type())
#define IM_ACCOUNT_PROTOCOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_ACCOUNT_PROTOCOL,ImAccountProtocol))
#define IM_ACCOUNT_PROTOCOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_ACCOUNT_PROTOCOL,ImAccountProtocolClass))
#define IM_IS_ACCOUNT_PROTOCOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_ACCOUNT_PROTOCOL))
#define IM_IS_ACCOUNT_PROTOCOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_ACCOUNT_PROTOCOL))
#define IM_ACCOUNT_PROTOCOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_ACCOUNT_PROTOCOL,ImAccountProtocolClass))

typedef struct _ImAccountProtocol      ImAccountProtocol;
typedef struct _ImAccountProtocolClass ImAccountProtocolClass;

struct _ImAccountProtocol {
	ImProtocol parent;
};

struct _ImAccountProtocolClass {
	ImProtocolClass parent_class;

};

/**
 * im_account_protocol_get_type:
 *
 * Returns: GType of the account protocol type
 */
GType  im_account_protocol_get_type   (void) G_GNUC_CONST;

/**
 * im_account_protocol_new:
 *
 * creates a new instance of #ImAccountProtocol
 *
 * Returns: a #ImAccountProtocol
 */
ImProtocol*    im_account_protocol_new (const gchar *name, const gchar *display_name, 
					guint port, guint alternate_port,
					GType account_g_type);

/**
 * im_account_protocol_get_port:
 * @self: a #ImAccountProtocol
 *
 * get the protocol standard port
 *
 * Returns: a string
 */
guint im_account_protocol_get_port (ImAccountProtocol *self);

/**
 * im_account_protocol_set_port:
 * @self: a #ImAccountProtocol
 * @port: a #guint
 *
 * set @port as the protocol standard port
 */
void         im_account_protocol_set_port (ImAccountProtocol *self,
					       guint port);

/**
 * im_account_protocol_get_alternate_port:
 * @self: a #ImAccountProtocol
 *
 * get the protocol standard alternate_port
 *
 * Returns: a #guint
 */
guint im_account_protocol_get_alternate_port (ImAccountProtocol *self);

/**
 * im_account_protocol_set_alternate_port:
 * @self: a #ImAccountProtocol
 * @alternate_port: a #guint
 *
 * set @alternate_port as the protocol alternate port
 */
void         im_account_protocol_set_alternate_port (ImAccountProtocol *self,
						     guint alternate_port);

/**
 * im_account_protocol_get_account_option_keys:
 * @self: a #ImAccountProtocol
 *
 * Obtains the list of options available for this protocol
 *
 * Returns: (transfer container): a list of the option keys set in the protocol
 */
GList *      im_account_protocol_get_account_option_keys (ImAccountProtocol *self);

/**
 * im_account_protocol_set_account_option:
 * @self: a #ImAccountProtocol
 * @key: a const string
 * @value: the value of the option
 *
 * set the account option that will be passed on creating the account
 */
void im_account_protocol_set_account_option (ImAccountProtocol *self,
					     const char *key, const char *value);

/**
 * im_account_protocol_get_account_option:
 * @self: a #ImAccountProtocol
 *
 * obtain the value of the account option
 *
 * Returns: (transfer none): the account options list.
 */
const char* im_account_protocol_get_account_option (ImAccountProtocol *self, const char *key);

/**
 * im_account_protocol_has_custom_secure_auth_mech:
 * @self: a #ImAccountProtocol
 * @auth_protocol_type: a #ImProtocolType for an auth protocol
 *
 * checks whether there's a custom secure auth mech camel string for @auth_protocol_type.
 *
 * Returns: %TRUE if registered, %FALSE otherwise
 */
gboolean
im_account_protocol_has_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type);

/**
 * im_account_protocol_get_custom_secure_auth_mech:
 * @self: a #ImAccountProtocol
 * @auth_protocol_type: a #ImProtocolType for an auth protocol
 *
 * obtains the secure auth mech of @auth_protocol_type in protocol. Be careful as %NULL does not imply
 * there's no custom auth mech registered (you can register %NULL). To check if it's registered, just
 * use im_account_protocol_has_custom_secure_auth_mech().
 *
 * Returns: the secure auth mech for this auth protocol type that will be passed to camel.
 */
const gchar *
im_account_protocol_get_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type);

/**
 * im_account_protocol_unset_custom_secure_auth_mech:
 * @self: a #ImAccountProtocol
 * @auth_protocol_type: a #ImProtocolType for an auth protocol
 *
 * Unsets the secure auth meth of @auth_protocol_type in protocol.
 */
void
im_account_protocol_unset_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type);

/**
 * im_account_protocol_set_custom_secure_auth_mech:
 * @self: a #ImAccountProtocol
 * @auth_protocol_type: a #ImProtocolType for an auth protocol
 * @secure_auth_mech: a string or %NULL
 *
 * sets the secure auth mech of @auth_protocol_type in protocol. Be careful as %NULL does not imply
 * there's no custom auth mech registered (you can register %NULL). If you set %NULL you're regitering %NULL as the custom secure auth
 * mechanism instead of unsetting it.
 */
void
im_account_protocol_set_custom_secure_auth_mech (ImAccountProtocol *self, ImProtocolType auth_protocol_type, const gchar *secure_auth_mech);

/**
 * im_account_protocol_get_account_g_type:
 * @self: a #ImAccountProtocol
 *
 * get the protocol type used for factoring new TnyAccount
 *
 * Returns: a #GType
 */
GType im_account_protocol_get_account_g_type (ImAccountProtocol *self);

/**
 * im_account_protocol_set_account_g_type:
 * @self: a #ImAccountProtocol
 * @account_g_type: a #GType
 *
 * set @account_g_type as the type im_account_protocol_create_account will
 * instanciate
 */
void         im_account_protocol_set_account_g_type (ImAccountProtocol *self,
							 GType account_g_type);

/**
 * im_account_protocol_create_account:
 * @self: a #ImAccountProtocol
 *
 * create a new account instance for this protocol
 *
 * Returns: a #TnyAccount
 */
GObject * im_account_protocol_create_account (ImAccountProtocol *self);

G_END_DECLS

#endif /* __IM_ACCOUNT_PROTOCOL_H__ */
