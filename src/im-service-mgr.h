/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-service-mgr.h : CamelService stores and transports manager */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Alberto Garcia <agarcia@igalia.com>
 *  Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
 *  Murray Cumming <murrayc@murrayc.com>
 *  Silvan Marco Fin <silvan@kernelconcepts.de>
 *  Nils Faerber <nils@kernelconcepts.de>
 *  Philip Van Hoof <philip@codeminded.be>
 *  Arne Zellentin <arne@kernelconcepts.de>
 *  Florian Boor <florian@kernelconcepts.de>
 *  Javier Jardón <javierjc1982@gmail.com>
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


/* modest-tny-account-store.h */

#ifndef __IM_SERVICE_MGR_H__
#define __IM_SERVICE_MGR_H__

#include <im-account-mgr.h>

#include <camel/camel.h>

/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_SERVICE_MGR             (im_service_mgr_get_type())
#define IM_SERVICE_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_SERVICE_MGR,ImServiceMgr))
#define IM_SERVICE_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_SERVICE_MGR,ImServiceMgrClass))
#define IM_IS_SERVICE_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_SERVICE_MGR))
#define IM_IS_SERVICE_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_SERVICE_MGR))
#define IM_SERVICE_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_SERVICE_MGR,ImServiceMgrClass))

typedef struct _ImServiceMgr      ImServiceMgr;
typedef struct _ImServiceMgrClass ImServiceMgrClass;

struct _ImServiceMgr {
	CamelSession parent;
};

struct _ImServiceMgrClass {
	CamelSessionClass parent_class;

	/* CamelSession vmethods */
	gchar *		(*get_password)		(CamelSession *session,
						 CamelService *service,
						 const gchar *prompt,
						 const gchar *item,
						 guint32 flags,
						 GError **error);

	gboolean	(*forget_password)	(CamelSession *session,
						 CamelService *service,
						 const gchar *item,
						 GError **error);
	gint		(*alert_user)		(CamelSession *session,
						 CamelSessionAlertType type,
						 const gchar *prompt,
						 GSList *button_captions);
	CamelFilterDriver *
			(*get_filter_driver)	(CamelSession *session,
						 const gchar *type,
						 GError **error);
	gboolean	(*forward_to)		(CamelSession *session,
						 CamelFolder *folder,
						 CamelMimeMessage *message,
						 const gchar *address,
						 GError **error);

	void (*account_update)        (ImServiceMgr *self,
				      const gchar *account_name,
				      gpointer user_data);
	void (*password_requested)    (ImServiceMgr *self,
				       const gchar *server_account_name,
				       gchar **username,
				       gchar **password,
				       gboolean *remember,
				       gboolean *cancel,
				       gpointer user_data);

	/* Signals */
	void (*service_changed) (ImServiceMgr *self, CamelService *account);
	void (*service_inserted) (ImServiceMgr *self, CamelService *account);
	void (*service_removed) (ImServiceMgr *self, CamelService *account);
};

/* We set 5Mb as the upper limit to consider disk full conditions */
#define IM_SERVICE_MGR_MIN_FREE_SPACE 5 * 1024 * 1024

/**
 * im_service_mgr_get_type:
 *
 * Returns: GType of the account store
 */
GType  im_service_mgr_get_type   (void) G_GNUC_CONST;

/**
 * im_service_mgr_get_instance:
 *
 * obtains the singleton #ImServiceMgr
 *
 * Returns: (transfer none): an #ImServiceMgr
 */
ImServiceMgr*    im_service_mgr_get_instance (void);


/**
 * im_service_mgr_get_service:
 * @self: a #ImServiceMgr instance
 * @account_id: an account id
 * @type: the account type (#IM_ACCOUNT_TYPE_STORE or #IM_ACCOUNT_TYPE_TRANSPORT)
 * 
 * Get the service corresponding to one of the server_accounts for account with @account_id
 * 
 * Returns: (transfer full): a #CamelService or %NULL
 */
CamelService* im_service_mgr_get_service (ImServiceMgr *self,
					  const gchar *account_id,
					  ImAccountType type);

const char *im_service_mgr_get_user_data_dir (void);

G_END_DECLS

#endif /* __IM_SERVICE_MGR_H__ */
