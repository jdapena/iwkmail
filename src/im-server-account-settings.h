/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-server-account-settings.h : settings for a CamelService */

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


#ifndef __IM_SERVER_ACCOUNT_SETTINGS_H__
#define __IM_SERVER_ACCOUNT_SETTINGS_H__

#include <im-protocol.h>

#include <glib-object.h>

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_SERVER_ACCOUNT_SETTINGS             (im_server_account_settings_get_type())
#define IM_SERVER_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettings))
#define IM_SERVER_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettingsClass))
#define IM_IS_SERVER_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS))
#define IM_IS_SERVER_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_SERVER_ACCOUNT_SETTINGS))
#define IM_SERVER_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_SERVER_ACCOUNT_SETTINGS,ImServerAccountSettingsClass))

typedef struct _ImServerAccountSettings      ImServerAccountSettings;
typedef struct _ImServerAccountSettingsClass ImServerAccountSettingsClass;

struct _ImServerAccountSettings {
	GObject parent;
};

struct _ImServerAccountSettingsClass {
	GObjectClass parent_class;
};


GType                       im_server_account_settings_get_type              (void) G_GNUC_CONST;

ImServerAccountSettings*    im_server_account_settings_new                   (void);

const gchar *               im_server_account_settings_get_account_name      (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_account_name      (ImServerAccountSettings *settings,
									      const gchar *account_name);
const gchar*                im_server_account_settings_get_hostname          (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_hostname          (ImServerAccountSettings *settings,
									      const gchar *hostname);
guint                       im_server_account_settings_get_port              (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_port              (ImServerAccountSettings *settings,
									      guint port);
ImProtocolType              im_server_account_settings_get_protocol          (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_protocol          (ImServerAccountSettings *settings,
									      ImProtocolType protocol_type);
ImProtocolType              im_server_account_settings_get_security_protocol (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_security_protocol (ImServerAccountSettings *settings,
									      ImProtocolType security_protocol);
ImProtocolType              im_server_account_settings_get_auth_protocol     (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_auth_protocol     (ImServerAccountSettings *settings,
									      ImProtocolType auth_protocol);
const gchar *               im_server_account_settings_get_username          (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_username          (ImServerAccountSettings *settings,
									      const gchar *username);
const gchar *               im_server_account_settings_get_password          (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_password          (ImServerAccountSettings *settings,
									      const gchar *password);
const gchar *               im_server_account_settings_get_uri               (ImServerAccountSettings *settings);
void                        im_server_account_settings_set_uri               (ImServerAccountSettings *settings,
									      const gchar *uri);

G_END_DECLS

#endif /* __IM_SERVER_ACCOUNT_SETTINGS_H__ */
