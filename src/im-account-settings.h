/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-settings.h : in memory representation of an account settings */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2007, Nokia Corporation
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

#ifndef __IM_ACCOUNT_SETTINGS_H__
#define __IM_ACCOUNT_SETTINGS_H__

#include <im-server-account-settings.h>

#include <glib-object.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_ACCOUNT_SETTINGS             (im_account_settings_get_type())
#define IM_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettings))
#define IM_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettingsClass))
#define IM_IS_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_ACCOUNT_SETTINGS))
#define IM_IS_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_ACCOUNT_SETTINGS))
#define IM_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_ACCOUNT_SETTINGS,ImAccountSettingsClass))

typedef struct _ImAccountSettings      ImAccountSettings;
typedef struct _ImAccountSettingsClass ImAccountSettingsClass;

struct _ImAccountSettings {
	GObject parent;
};

struct _ImAccountSettingsClass {
	GObjectClass parent_class;
};

typedef enum {
	IM_ACCOUNT_TYPE_STORE = 0,
	IM_ACCOUNT_TYPE_TRANSPORT
} ImAccountType;

typedef enum {
	IM_ACCOUNT_RETRIEVE_HEADERS_ONLY = 0,
	IM_ACCOUNT_RETRIEVE_MESSAGES,
	IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS
} ImAccountRetrieveType;

GType                    im_account_settings_get_type               (void) G_GNUC_CONST;
ImAccountSettings*       im_account_settings_new                    (void);

const gchar *            im_account_settings_get_id                 (ImAccountSettings *settings);
void                     im_account_settings_set_id                 (ImAccountSettings *settings,
								     const gchar *id);
const gchar*             im_account_settings_get_display_name       (ImAccountSettings *settings);
void                     im_account_settings_set_display_name       (ImAccountSettings *settings,
								     const gchar *display_name);
gboolean                 im_account_settings_get_enabled            (ImAccountSettings *settings);
void                     im_account_settings_set_enabled            (ImAccountSettings *settings,
								     gboolean enabled);
gboolean                 im_account_settings_get_is_default         (ImAccountSettings *settings);
void                     im_account_settings_set_is_default         (ImAccountSettings *settings,
								     gboolean is_default);
const gchar*             im_account_settings_get_fullname           (ImAccountSettings *settings);
void                     im_account_settings_set_fullname           (ImAccountSettings *settings,
								     const gchar *fullname);
const gchar*             im_account_settings_get_email_address      (ImAccountSettings *settings);
void                     im_account_settings_set_email_address      (ImAccountSettings *settings,
								     const gchar *email_address);
gboolean                 im_account_settings_get_use_signature      (ImAccountSettings *settings);
void                     im_account_settings_set_use_signature      (ImAccountSettings *settings,
								     gboolean use_signature);
const gchar*             im_account_settings_get_signature          (ImAccountSettings *settings);
void                     im_account_settings_set_signature          (ImAccountSettings *settings,
								     const gchar *signature);
ImAccountRetrieveType    im_account_settings_get_retrieve_type      (ImAccountSettings *settings);
void                     im_account_settings_set_retrieve_type      (ImAccountSettings *settings,
								     ImAccountRetrieveType retrieve_type);
guint                    im_account_settings_get_retrieve_limit     (ImAccountSettings *settings);
void                     im_account_settings_set_retrieve_limit     (ImAccountSettings *settings,
								     guint retrieve_limit);
gboolean                 im_account_settings_get_leave_messages_on_server (ImAccountSettings *settings);
void                     im_account_settings_set_leave_messages_on_server (ImAccountSettings *settings, 
									   gboolean leave_messages_on_server);
ImServerAccountSettings *im_account_settings_get_store_settings     (ImAccountSettings *settings);
void                     im_account_settings_set_store_settings     (ImAccountSettings *settings, 
								     ImServerAccountSettings *store_settings);
ImServerAccountSettings *im_account_settings_get_transport_settings (ImAccountSettings *settings);
void                     im_account_settings_set_transport_settings (ImAccountSettings *settings, 
								     ImServerAccountSettings *transport_settings);

G_END_DECLS

#endif /* __IM_ACCOUNT_SETTINGS_H__ */
