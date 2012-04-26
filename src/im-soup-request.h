/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright (C) 2011, Igalia S.L.
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
