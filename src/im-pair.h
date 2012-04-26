/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-pair.h : a pair of strings */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Sergio Villar Senin <svillar@igalia.com>
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

#ifndef __IM_PAIR_H__
#define __IM_PAIR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _ImPair {
	gpointer *first;
	gpointer *second;
	gboolean own;
} ImPair;

typedef GSList ImPairList;


/**
 * im_pair_new:
 * @first: the first element of the pair
 * @second: the second element of the pair
 * @own: does the pair own the item (ie. should the items be freed when the pair is destroyed)
 *
 * create a new ImPair instance
 *
 * Returns: a newly allocated ImPair instance
 */
ImPair* im_pair_new   (gpointer first, gpointer second, gboolean own);



/**
 * im_pair_free:
 * @self: a valid ImPair instance or NULL
 *
 * free a ImPair instance. If it was created with own==TRUE, the elements
 * will be g_free'd as well. If pair==NULL, nothing will be done.
 */
void        im_pair_free       (ImPair *self);


/**
 * im_pair_list_free:
 * @pairs: a valid ImPairList
 *
 * Convenience function to destroy all pairs in a list and the list itself.
 *
 */
void  im_pair_list_free (ImPairList *pairs);

/**
 * im_pair_list_find_by_first
 * @pairs: A valid ImPairList
 * @first: The first element of the pair to be found.
 * @func: The function to call for each element. It should return 0 when the desired element is found.
 *
 * Find an element with a matching first entry.
 *
 * Returns: a previously allocated ImPair instance, or NULL if none was found.
 */
ImPair* im_pair_list_find_by_first_as_string  (ImPairList *pairs, 
	const gchar* first);

G_END_DECLS

#endif /* __IM_PAIR_H__ */
