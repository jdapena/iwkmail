/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-soup-request.h : SoupRequest implementing iwk: protocol, invoked from JS code */

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

#ifndef IM_SOUP_REQUEST_H
#define IM_SOUP_REQUEST_H 1

#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-request.h>

#define IM_TYPE_SOUP_REQUEST            (im_soup_request_get_type ())
#define IM_SOUP_REQUEST(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), IM_TYPE_SOUP_REQUEST, ImSoupRequest))
#define IM_SOUP_REQUEST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IM_TYPE_SOUP_REQUEST, ImSoupRequestClass))
#define EPHY_IS_REQUEST_ABOUT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), IM_TYPE_SOUP_REQUEST))
#define EPHY_IS_REQUEST_ABOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IM_TYPE_SOUP_REQUEST))
#define IM_SOUP_REQUEST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IM_TYPE_SOUP_REQUEST, ImSoupRequestClass))

#define IM_SCHEME "iwk"

typedef struct _ImSoupRequestPrivate ImSoupRequestPrivate;

typedef struct {
  SoupRequest parent;

  ImSoupRequestPrivate *priv;
} ImSoupRequest;

typedef struct {
  SoupRequestClass parent;

} ImSoupRequestClass;

GType im_soup_request_get_type (void);

#endif /* IM_SOUP_REQUEST_H */
