/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-window.h : Application window */

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

#ifndef IM_WINDOW_H
#define IM_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IM_TYPE_WINDOW	        (im_window_get_type ())
#define IM_WINDOW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), IM_TYPE_WINDOW, ImWindow))
#define IM_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), IM_TYPE_WINDOW, ImWindowClass))
#define IM_IS_WINDOW(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), IM_TYPE_WINDOW))
#define IM_IS_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), IM_TYPE_WINDOW))
#define IM_WINDOW_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IM_TYPE_WINDOW, ImWindowClass))

typedef struct _ImWindowClass		ImWindowClass;
typedef struct _ImWindow		ImWindow;
typedef struct _ImWindowPrivate	        ImWindowPrivate;

struct _ImWindow
{
	GtkApplicationWindow parent;

	/*< private >*/
	ImWindowPrivate *priv;
};

struct _ImWindowClass
{
	GtkApplicationWindowClass parent_class;
};

GType		  im_window_get_type		  (void);

GtkWidget	 *im_window_new		          (GtkApplication *window);

G_END_DECLS

#endif
