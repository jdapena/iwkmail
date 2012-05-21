/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-mail-ops.h : Asynchronous mail operations */

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

#ifndef IM_MAIL_OPS_H
#define IM_MAIL_OPS_H 1

#include "im-service-mgr.h"

#include <camel/camel.h>
#include <glib.h>

G_BEGIN_DECLS

gboolean          im_mail_op_flag_message_sync            (ImServiceMgr *service_mgr,
							   const gchar *account_id,
							   const gchar *folder_name,
							   const gchar *message_uid,
							   const gchar *set_flags,
							   const gchar *unset_flags,
							   GCancellable *cancellable,
							   GError **error);
void              im_mail_op_flag_message_async           (ImServiceMgr *mgr,
							   const gchar *account_id,
							   const gchar *folder_name,
							   const gchar *message_uid,
							   const gchar *set_flags,
							   const gchar *unset_flags,
							   int io_priority,
							   GCancellable *cancellable,
							   GAsyncReadyCallback callback,
							   gpointer userdata);
gboolean          im_mail_op_flag_message_finish          (ImServiceMgr *mgr,
							   GAsyncResult *result,
							   GError **error);

gboolean          im_mail_op_composer_save_sync           (CamelFolder *destination,
							   CamelMimeMessage *message,
							   const gchar *body,
							   GList *attachment_uris,
							   gchar **uid,
							   GCancellable *cancellable,
							   GError **error);
void              im_mail_op_composer_save_async          (CamelFolder *folder,
							   CamelMimeMessage *message,
							   const gchar *body,
							   GList *attachment_uris,
							   int io_priority,
							   GCancellable *cancellable,
							   GAsyncReadyCallback callback,
							   gpointer userdata);
gboolean          im_mail_op_composer_save_finish         (CamelFolder *folder,
							   GAsyncResult *result,
							   gchar **uid,
							   GError **error);


G_END_DECLS

#endif /* IM_MAIL_OPS_H */
