/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-file-utils.c : file helpers */

/*
 * Authors:
 *  Sergio Villar Senin <svillar@igalia.com>
 *  Jose Dapena Paz <jdapena@igalia.com>
 *  Alberto Garcia <agarcia@igalia.com>
 *  Javier Fernandez Garcia-Boente <jfernandez@igalia.com>
 *  Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
 *  Arne Zellentin <arne@kernelconcepts.de>
 *  Alexander Chumakov <alexander.chumakov@teleca.com>
 *  Murray Cumming <murrayc@murrayc.com>
 *  Johannes Schmid <johannes.schmid@openismus.com>
 *  Lucas Maneos <maemo@subs.maneos.org>
 *  Nils Faerber <nils@kernelconcepts.de>
 *
 * Copyright (c) 2012, Igalia, S.L.
 *
 * Work derived from Modest:
 * Copyright (c) 2006-2011 Nokia Corporation
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

#include <config.h>
#include "im-file-utils.h"

#include <errno.h>
#include <fcntl.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * im_file_utils_folder_writable:
 * @filename: a string
 *
 * Checks if @filename is in a writable folder
 *
 * Returns: %TRUE if @filename is writable, %FALSE otherwise
 */
gboolean
im_file_utils_folder_writable (const gchar *filename)
{
	gboolean result = TRUE;
	g_return_val_if_fail (filename, FALSE);

	if (!filename)
		return FALSE;

	if (g_ascii_strncasecmp (filename, "obex", 4) != 0) {
		GFile *file;
		GFile *parent_file;

		file = g_file_new_for_uri (filename);
		parent_file = g_file_get_parent (file);
		
		if (parent_file) {
			GFileInfo *folder_info;

			folder_info = g_file_query_info (parent_file, 
							 G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
							 G_FILE_QUERY_INFO_NONE, NULL, NULL);
			if (folder_info) {
				result = g_file_info_get_attribute_boolean (folder_info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
				g_object_unref (parent_file);
			}
			g_object_unref (parent_file);
		}
		g_object_unref (file);

	}
	return result;
}

/**
 * im_file_utils_file_exists:
 * @filename: a string
 *
 * Checks if @filename exists. The filename must NOT use escaped
 * characters as this function uses g_access to check if the file
 * exists or not
 *
 * Returns: %TRUE if @filename currently exists, %FALSE otherwise
 */
gboolean
im_file_utils_file_exists (const gchar *filename)
{
	gboolean result = FALSE;
	gchar *path;

	g_return_val_if_fail (filename, FALSE);

	path = strstr (filename, "file://");
	if (!path)
		path = (gchar *) filename;
	else
		path = (gchar *) filename + strlen ("file://");

	if (g_access (path, F_OK) == 0)
		result = TRUE;

	return result;
}

/**
 * im_file_utils_create_temp_uri:
 * @orig_name: a string with the original name of the extension, or %NULL
 * @hash_base: if %NULL, subdir will be random. If not, it will be a hash
 * of this.
 *
 * Creates a temporary file, in a random subdir of temp dir.
 *
 * Returns: (transfer full): URI of the file, or %NULL if operation failed.  
 */
gchar *
im_file_utils_create_temp_uri (const gchar *orig_name,
			       const gchar *hash_base)
{
	gchar *filepath;
	gchar *tmpdir;
	guint hash_number;
	gchar *uri;

	/* hmmm... maybe we need a modest_text_utils_validate_file_name? */
	g_return_val_if_fail (orig_name && strlen(orig_name) != 0, NULL);
	if (strlen(orig_name) > 254) {
		g_warning (_("%s: filename too long ('%s')"),
			   __FUNCTION__, orig_name);
		return NULL;
	}

	if (g_strstr_len (orig_name, strlen (orig_name), "/") != NULL) {
		g_warning (_("%s: filename contains '/' character(s) (%s)"),
			   __FUNCTION__, orig_name);
		return NULL;
	}

	/* make a random subdir under temp dir */
	if (hash_base != NULL) {
		hash_number = g_str_hash (hash_base);
	} else {
		hash_number = (guint) g_random_int ();
	}
	tmpdir = g_strdup_printf ("%s/%u", g_get_tmp_dir (), hash_number);
	if ((g_access (tmpdir, R_OK) == -1) && (g_mkdir (tmpdir, 0755) == -1)) {
		g_warning (_("%s: failed to create dir '%s': %s"),
			   __FUNCTION__, tmpdir, g_strerror(errno));
		g_free (tmpdir);
		return NULL;
	}

	filepath = g_build_filename (tmpdir, orig_name, NULL);
	g_free (tmpdir);

	/* if file exists, first we try to remove it */
	if (g_access (filepath, F_OK) == 0)
		g_unlink (filepath);

	/* don't overwrite if it already exists, even if it is writable */
	if (g_access (filepath, F_OK) != 0) {
		gint fd;
		/* try to write the file there */
		fd = g_open (filepath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
		if (fd == -1) {
			g_warning (_("%s: failed to create '%s': %s"),
				   __FUNCTION__, filepath, g_strerror(errno));
			g_free (filepath);
			return NULL;
		} else {
			close (fd);
		}
	}

	uri = g_filename_to_uri (filepath, NULL, NULL);
	g_free (filepath);

	return uri;
}

/**
 * im_file_utils_get_available_space:
 * @folder_path: the path of the folder
 *
 * Obtains the space available in the local folder.
 *
 * Returns: a #guint64
 */
guint64 
im_file_utils_get_available_space (const gchar *folder_path)
{
	GFile *file;
	GFileInfo *info;
	guint64 size;

	file = g_file_new_for_path (folder_path);
	info = g_file_query_filesystem_info (file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
					     NULL, NULL);
	if  (info) {
		size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
		g_object_unref (info);
	} else {
		size = 0;
	}
	g_object_unref (file);

	return (guint64) size;
}

