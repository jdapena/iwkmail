/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-error.h : Error handling */

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

#ifndef __IM_ERROR_H__
#define __IM_ERROR_H__

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define IM_ERROR_DOMAIN (im_get_error_quark ())

typedef enum {
	IM_ERROR_INTERNAL,
	IM_ERROR_CONF_INVALID_VALUE,
	IM_ERROR_SOUP_INVALID_URI,
	IM_ERROR_ACCOUNT_MGR_ADD_ACCOUNT_FAILED,
	IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED,
	IM_ERROR_ACCOUNT_MGR_GET_ACCOUNTS_FAILED,
	IM_ERROR_SERVICE_MGR_FLAG_MESSAGE_FAILED,
	IM_ERROR_SETTINGS_INVALID_ACCOUNT_NAME,
	IM_ERROR_SETTINGS_INVALID_AUTH_PROTOCOL,
	IM_ERROR_SETTINGS_INVALID_CONNECTION_PROTOCOL,
	IM_ERROR_SETTINGS_INVALID_EMAIL_ADDRESS,
	IM_ERROR_SETTINGS_INVALID_HOST,
	IM_ERROR_SETTINGS_INVALID_PROTOCOL,
	IM_ERROR_SETTINGS_INVALID_USERNAME,
	IM_ERROR_SEND_INVALID_PARAMETERS,
	IM_ERROR_SEND_NO_RECIPIENTS,
	IM_ERROR_SEND_PARSING_RECIPIENTS,
	IM_ERROR_SEND_INVALID_ACCOUNT_FROM,
	IM_ERROR_SEND_FAILED_TO_ADD_TO_OUTBOX,
	IM_ERROR_SEND_INVALID_ATTACHMENT,
	IM_ERROR_COMPOSER_FAILED_TO_ADD_TO_DRAFTS,
	IM_ERROR_AUTH_FAILED
} ImErrorCode;

GQuark im_get_error_quark (void);


G_END_DECLS

#endif /* __IM_ERROR_H__ */
