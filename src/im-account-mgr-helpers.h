/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-account-mgr-helpers.h : helpers methods for dealing with ImAccountMgr */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Alberto Garcia Gonzalez <agarcia@igalia.com>
 *  Dirk-Jan C. Binnema
 *  Murray Cumming
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2006, Nokia Corporation
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
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMIT
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


#ifndef __IM_ACCOUNT_MGR_HELPERS_H__
#define __IM_ACCOUNT_MGR_HELPERS_H__

#include <im-account-mgr.h>
#include <im-account-settings.h>
#include <im-server-account-settings.h>

G_BEGIN_DECLS

ImAccountSettings * im_account_mgr_load_account_settings           (ImAccountMgr *self,
								    const gchar* name);
void                im_account_mgr_save_account_settings           (ImAccountMgr *self,
								    ImAccountSettings *settings);
ImProtocolType      im_account_mgr_get_store_protocol              (ImAccountMgr *self,
								    const gchar* name);

gboolean            im_account_mgr_set_first_account_as_default    (ImAccountMgr *self);
gchar *             im_account_mgr_get_first_account_name          (ImAccountMgr *self);


gboolean            im_account_mgr_set_enabled                     (ImAccountMgr *self,
								    const gchar* name,
								    gboolean enabled);
gboolean            im_account_mgr_get_enabled                     (ImAccountMgr *self,
								    const gchar* name);
gboolean            im_account_mgr_set_signature                   (ImAccountMgr *self,
								    const gchar* name, 
								    const gchar* signature,
								    gboolean use_signature);
gchar *             im_account_mgr_get_signature                   (ImAccountMgr *self,
								    const gchar* name,
								    gboolean* use_signature);

gchar *             im_account_mgr_get_server_account_username     (ImAccountMgr *self, 
								    const gchar* account_name);
void                im_account_mgr_set_server_account_username     (ImAccountMgr *self, 
								    const gchar* account_name, 
								    const gchar* username);
gboolean            im_account_mgr_get_server_account_username_has_succeeded (ImAccountMgr *self, 
									      const gchar* account_name);
void                im_account_mgr_set_server_account_username_has_succeeded (ImAccountMgr *self, 
									      const gchar* account_name, 
									      gboolean succeeded);
gchar *             im_account_mgr_get_server_account_password     (ImAccountMgr *self, 
								    const gchar* account_name);
gboolean            im_account_mgr_get_server_account_has_password (ImAccountMgr *self, 
								    const gchar* account_name);

gchar *             im_account_mgr_get_server_account_hostname     (ImAccountMgr *self, 
								    const gchar* account_name);
void                im_account_mgr_set_server_account_hostname     (ImAccountMgr *self, 
								    const gchar* account_name,
								    const gchar *hostname);
ImProtocolType      im_account_mgr_get_server_account_secure_auth  (ImAccountMgr *self,
								    const gchar* account_name);
void                im_account_mgr_set_server_account_secure_auth  (ImAccountMgr *self, 
								    const gchar* account_name, 
								    ImProtocolType secure_auth);
ImProtocolType      im_account_mgr_get_server_account_security     (ImAccountMgr *self, 
								    const gchar* account_name);
void                im_account_mgr_set_server_account_security     (ImAccountMgr *self, 
								    const gchar* account_name, 
								    ImProtocolType security);

gboolean            im_account_mgr_save_server_settings            (ImAccountMgr *self,
								    ImServerAccountSettings *settings);
ImServerAccountSettings *im_account_mgr_load_server_settings       (ImAccountMgr *self,
								    const gchar *account_name,
								    gboolean is_transport_not_store);

gchar *             im_account_mgr_get_from_string                 (ImAccountMgr *self,
								    const gchar* name,
								    const gchar *mailbox);
gchar *             im_account_mgr_get_unused_account_name         (ImAccountMgr *self, 
								    const gchar* starting_name,
								    gboolean server_account);
gchar *             im_account_mgr_get_unused_account_display_name (ImAccountMgr *self, 
								    const gchar* starting_name);

void                im_account_mgr_set_leave_on_server             (ImAccountMgr *self, 
								    const gchar* account_name, 
								    gboolean leave_on_server);
gboolean            im_account_mgr_get_leave_on_server             (ImAccountMgr *self, 
								    const gchar* account_name);
gint                im_account_mgr_get_last_updated                (ImAccountMgr *self, 
								    const gchar* account_name);

void                im_account_mgr_set_last_updated                (ImAccountMgr *self, 
								    const gchar* account_name,
								    gint time);
gboolean            im_account_mgr_get_has_new_mails               (ImAccountMgr *self, 
								    const gchar* account_name);
void                im_account_mgr_set_has_new_mails               (ImAccountMgr *self, 
								    const gchar* account_name,
								    gboolean has_new_mails);
gint                im_account_mgr_get_retrieve_limit              (ImAccountMgr *self, 
								    const gchar* account_name);
void                im_account_mgr_set_retrieve_limit             (ImAccountMgr *self, 
								   const gchar* account_name,
								   gint limit_retrieve);
gint                im_account_mgr_get_server_account_port        (ImAccountMgr *self, 
								   const gchar* account_name);
void                im_account_mgr_set_server_account_port        (ImAccountMgr *self, 
								   const gchar *account_name,
								   gint port_num);
gchar *             im_account_mgr_get_server_account_name       (ImAccountMgr *self, 
								  const gchar *account_name,
								  ImAccountType account_type);
gchar *             im_account_mgr_get_server_parent_account_name(ImAccountMgr *self,
								  const char *server_account_name,
								  ImAccountType account_type);
ImAccountRetrieveType im_account_mgr_get_retrieve_type           (ImAccountMgr *self, 
								  const gchar *account_name);
void                im_account_mgr_set_retrieve_type             (ImAccountMgr *self, 
								  const gchar *account_name,
								  ImAccountRetrieveType retrieve_type);
void                im_account_mgr_set_user_fullname             (ImAccountMgr *self, 
								  const gchar *account_name,
								  const gchar *fullname);
void                im_account_mgr_set_user_email                (ImAccountMgr *self, 
								  const gchar *account_name,
								  const gchar *email);

G_END_DECLS

#endif /* __IM_ACCOUNT_MGR_H__ */
