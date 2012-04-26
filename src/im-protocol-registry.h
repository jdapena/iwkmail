/* Copyright (c) 2007, Nokia Corporation
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

#ifndef __IM_PROTOCOL_REGISTRY_H__
#define __IM_PROTOCOL_REGISTRY_H__

#include <glib-object.h>
#include "im-protocol.h"
#include "im-pair.h"

G_BEGIN_DECLS

#define IM_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS "connection"
#define IM_PROTOCOL_REGISTRY_AUTH_PROTOCOLS "auth"
#define IM_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS "transport-store"
#define IM_PROTOCOL_REGISTRY_STORE_PROTOCOLS "store"
#define IM_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS "transport"
#define IM_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS "local-store"
#define IM_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS "remote-store"
#define IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS "secure"
#define IM_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS "leave-on-server-available"
#define IM_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS "providers"
#define IM_PROTOCOL_REGISTRY_SINGLETON_PROVIDER_PROTOCOLS "singleton-providers"
#define IM_PROTOCOL_REGISTRY_MULTI_MAILBOX_PROVIDER_PROTOCOLS "multi-mailbox-providers"
#define IM_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT "use-alternate-port"
#define IM_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS "store-has-folders"
/* Accounts that cannot be the destination of messages or folders transfers */
#define IM_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS "store-forbid-incoming-xfers"
/* Accounts that do not allow to move messages or folders from */
#define IM_PROTOCOL_REGISTRY_STORE_FORBID_OUTGOING_XFERS "store-forbid-outgoing-xfers"
#define IM_PROTOCOL_REGISTRY_NO_AUTO_UPDATE_PROTOCOLS "no-auto-update"

/* convenience macros */
#define IM_TYPE_PROTOCOL_REGISTRY             (im_protocol_registry_get_type())
#define IM_PROTOCOL_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_PROTOCOL_REGISTRY,ImProtocolRegistry))
#define IM_PROTOCOL_REGISTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_PROTOCOL_REGISTRY,ImProtocolRegistryClass))
#define IM_IS_PROTOCOL_REGISTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_PROTOCOL_REGISTRY))
#define IM_IS_PROTOCOL_REGISTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_PROTOCOL_REGISTRY))
#define IM_PROTOCOL_REGISTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_PROTOCOL_REGISTRY,ImProtocolRegistryClass))

/* a special type, equivalent to a NULL protocol */
#define IM_PROTOCOL_REGISTRY_TYPE_INVALID -1

/* The minimum priority custom protocols should take for their index */
#define IM_PROTOCOL_REGISTRY_USER_PRIORITY 100

/* macros to access the default configured protocols */
#define IM_PROTOCOLS_TRANSPORT_SMTP (im_protocol_registry_get_smtp_type_id())
#define IM_PROTOCOLS_TRANSPORT_SENDMAIL (im_protocol_registry_get_sendmail_type_id())
#define IM_PROTOCOLS_STORE_POP (im_protocol_registry_get_pop_type_id())
#define IM_PROTOCOLS_STORE_IMAP (im_protocol_registry_get_imap_type_id())
#define IM_PROTOCOLS_STORE_MAILDIR (im_protocol_registry_get_maildir_type_id())
#define IM_PROTOCOLS_STORE_MBOX (im_protocol_registry_get_mbox_type_id())
#define IM_PROTOCOLS_CONNECTION_NONE (im_protocol_registry_get_none_connection_type_id ())
#define IM_PROTOCOLS_CONNECTION_TLS (im_protocol_registry_get_tls_connection_type_id ())
#define IM_PROTOCOLS_CONNECTION_SSL (im_protocol_registry_get_ssl_connection_type_id ())
#define IM_PROTOCOLS_CONNECTION_TLSOP (im_protocol_registry_get_tlsop_connection_type_id ())
#define IM_PROTOCOLS_AUTH_NONE (im_protocol_registry_get_none_auth_type_id ())
#define IM_PROTOCOLS_AUTH_PASSWORD (im_protocol_registry_get_password_auth_type_id ())
#define IM_PROTOCOLS_AUTH_CRAMMD5 (im_protocol_registry_get_crammd5_auth_type_id ())

/* properties available */
#define IM_PROTOCOL_SECURITY_ACCOUNT_OPTION "im-security-account-option"
#define IM_PROTOCOL_AUTH_ACCOUNT_OPTION "im-auth-account-option"

/* translations */
#define IM_PROTOCOL_TRANSLATION_DELETE_MAILBOX "translation-delete-mailbox" /* title string */
#define IM_PROTOCOL_TRANSLATION_CONNECT_ERROR "translation-connect-error" /* server name */
#define IM_PROTOCOL_TRANSLATION_AUTH_ERROR "translation-auth-error" /* server name */
#define IM_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR "translation-account-connection-error" /* hostname */
#define IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE "translation-msg-not-available" /* subject */
#define IM_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER "translation-msg-not-available-lost-header" /* subject */
#define IM_PROTOCOL_TRANSLATION_SSL_PROTO_NAME "translation-ssl-proto-name"


typedef struct _ImProtocolRegistry      ImProtocolRegistry;
typedef struct _ImProtocolRegistryClass ImProtocolRegistryClass;

typedef guint ImProtocolRegistryType;

struct _ImProtocolRegistry {
	GObject parent;
};

struct _ImProtocolRegistryClass {
	GObjectClass parent_class;
};

/**
 * im_protocol_registry_get_type:
 *
 * Returns: GType of the account store
 */
GType  im_protocol_registry_get_type   (void) G_GNUC_CONST;

/**
 * im_protocol_registry_get_instance:
 *
 * Obtains the singleton instance of #ImProtocolRegistry
 *
 * Returns: (transfer none): a #ImProtocolRegistry
 */
ImProtocolRegistry*    im_protocol_registry_get_instance (void);

/**
 * im_protocol_registry_add:
 * @self: a #ImProtocolRegistry
 * @protocol: a #ImProtocol
 * @priority: priority establishes the order the protocols will be shown on listings
 * @first_tag: a string
 * @...: a %NULL terminated list of strings with the tags for the protocol
 *
 * Add @protocol to the registry @self, setting the proper identifying tags
 */
void im_protocol_registry_add (ImProtocolRegistry *self, ImProtocol *protocol, gint priority, const gchar *first_tag, ...);

/**
 * im_protocol_registry_get_all:
 * @self: a #ImProtocolRegistry
 *
 * obtains a list of all protocols registered in @self
 *
 * Returns: a newly allocated GSList of the protocols. Don't unref the protocols, only the list.
 */
GSList *im_protocol_registry_get_all (ImProtocolRegistry *self);

/**
 * im_protocol_registry_get_by_tag:
 * @self: a #ImProtocolRegistry
 * @tag: a string
 *
 * obtains a list of all protocols registered in @self tagged with @tag
 *
 * Returns: a newly allocated GSList of the protocols. Don't unref the protocol, only the list.
 */
GSList *im_protocol_registry_get_by_tag (ImProtocolRegistry *self, const gchar *tag);

/**
 * im_protocol_registry_get_pair_list_by_tag:
 * @self: a #ImProtocolRegistry
 * @tag: a string
 *
 * obtains a pair list of all protocols registered in @self tagged with @tag
 *
 * Returns: a newly allocated #ImPairList of the protocols. Should be freed using
 * im_pair_list_free ()
 */
ImPairList *im_protocol_registry_get_pair_list_by_tag (ImProtocolRegistry *self, const gchar *tag);

/**
 * im_protocol_registry_get_protocol_by_name:
 * @self: a #ImProtocolRegistry
 * @tag: a string
 * @name: a string
 *
 * Obtains the protocol in registry @self, tagged with @tag and with @name
 *
 * Returns: the obtained #ImProtocol, or %NULL if not found.
 */
ImProtocol *im_protocol_registry_get_protocol_by_name (ImProtocolRegistry *self, const gchar *tag, const gchar *name);

/**
 * im_protocol_registry_get_protocol_by_type:
 * @self: a #ImProtocolRegistry
 * @type_id: a #ImProtocolType
 *
 * Obtains the protocol in registry @self, identified by @type_id
 *
 * Returns: the obtained #ImProtocol, or %NULL if not found.
 */
ImProtocol *im_protocol_registry_get_protocol_by_type (ImProtocolRegistry *self, ImProtocolType type_id);

/**
 * im_protocol_registry_protocol_type_has_tag:
 * @self: a #ImProtocolRegistry
 * @type_id: a #ImProtocolType
 * @tag: a string
 *
 * Checks if a protocol identified with @type_id has a specific @tag.
 *
 * Returns: %TRUE if @type_id protocol has @tag in registry @self
 */
gboolean im_protocol_registry_protocol_type_has_tag (ImProtocolRegistry *self, ImProtocolType type_id, const gchar *tag);

#define im_protocol_registry_protocol_type_is_secure(registry,protocol_type) \
	im_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							IM_PROTOCOL_REGISTRY_SECURE_PROTOCOLS)

#define im_protocol_registry_protocol_type_is_provider(registry,protocol_type) \
	im_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							IM_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS)

#define im_protocol_registry_protocol_type_is_singleton_provider(registry,protocol_type) \
	im_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							IM_PROTOCOL_REGISTRY_SINGLETON_PROVIDER_PROTOCOLS)

#define im_protocol_registry_protocol_type_has_leave_on_server(registry,protocol_type) \
	im_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							IM_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS)

/**
 * @self: a #ImProtocolRegistry
 *
 * Set default available protocols in Im in @self
 */
void im_protocol_registry_set_to_default (ImProtocolRegistry *self);

ImProtocolType im_protocol_registry_get_imap_type_id (void);
ImProtocolType im_protocol_registry_get_pop_type_id (void);
ImProtocolType im_protocol_registry_get_maildir_type_id (void);
ImProtocolType im_protocol_registry_get_mbox_type_id (void);
ImProtocolType im_protocol_registry_get_smtp_type_id (void);
ImProtocolType im_protocol_registry_get_sendmail_type_id (void);
ImProtocolType im_protocol_registry_get_none_connection_type_id (void);
ImProtocolType im_protocol_registry_get_tls_connection_type_id (void);
ImProtocolType im_protocol_registry_get_ssl_connection_type_id (void);
ImProtocolType im_protocol_registry_get_tlsop_connection_type_id (void);
ImProtocolType im_protocol_registry_get_none_auth_type_id (void);
ImProtocolType im_protocol_registry_get_password_auth_type_id (void);
ImProtocolType im_protocol_registry_get_crammd5_auth_type_id (void);

G_END_DECLS

#endif /* __IM_PROTOCOL_REGISTRY_H__ */
