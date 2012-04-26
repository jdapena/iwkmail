/* Based on ModestConf */

/* Copyright (c) 2006, Nokia Corporation
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

#ifndef __IM_CONF_H__
#define __IM_CONF_H__

#include <glib-object.h>

#define IM_NAMESPACE_ENV "IM_GCONF_NAMESPACE"
#define IM_DEFAULT_NAMESPACE "/apps/iwkmail"

G_BEGIN_DECLS

/* convenience macros */
#define IM_TYPE_CONF             (im_conf_get_type())
#define IM_CONF(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),IM_TYPE_CONF,ImConf))
#define IM_CONF_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),IM_TYPE_CONF,GObject))
#define IM_IS_CONF(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),IM_TYPE_CONF))
#define IM_IS_CONF_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),IM_TYPE_CONF))
#define IM_CONF_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),IM_TYPE_CONF,ImConfClass))

typedef struct _ImConf        ImConf;
typedef struct _ImConfClass   ImConfClass;

typedef guint ImConfNotificationId; 

typedef enum {
	IM_CONF_VALUE_INT,
	IM_CONF_VALUE_BOOL,
	IM_CONF_VALUE_FLOAT,
	IM_CONF_VALUE_STRING
} ImConfValueType;

typedef enum {
	IM_CONF_EVENT_KEY_CHANGED,
	IM_CONF_EVENT_KEY_UNSET
} ImConfEvent;

struct _ImConf {
	 GObject parent;
};

struct _ImConfClass {
	GObjectClass parent_class;	
	void (* key_changed) (ImConf* self, 
			      const gchar *key, 
			      ImConfEvent event,
			      ImConfNotificationId id);
};

/**
 * im_conf_get_type:
 * 
 * get the #GType for #ImConf
 *  
 * Returns: the #GType
 */
GType        im_conf_get_type    (void) G_GNUC_CONST;


/**
 * im_conf_get_instance:
 *
 * obtains the #ImConf singleton
 *
 * Returns: an #ImConf
 */
ImConf*     im_conf_get_instance (void);

/**
 * im_conf_namespace:
 * @string: suffix for the namespace in ImConf
 *
 * Returns: (transfer none): full namespace for @string
 */
const gchar *im_defs_namespace (const gchar *string);


/**
 * im_conf_get_string:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a string from the configuration system
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. @err gives details in case of error
 */
gchar*       im_conf_get_string  (ImConf* self, const gchar* key, GError **err);


/** 
 * im_conf_get_int:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get an integer from the configuration system
 *  
 * Returns: an integer with the value for the key, or -1 in case of error
 * (of course, -1 can also be returned in non-error cases).
 * @err gives details in case of error
 */
gint         im_conf_get_int     (ImConf* self, const gchar* key, GError **err);

/** 
 * im_conf_get_float:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get an integer from the configuration system
 *  
 * Returns: an double with the value for the key, or -1 in case of
 * error (of course, -1 can also be returned in non-error cases).
 * @err gives details in case of error
 */
gdouble      im_conf_get_float   (ImConf* self, const gchar* key, GError **err);

/** 
 * im_conf_get_bool:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a boolean value from the configuration system
 *  
 * Returns: a boolean value with the value for the key, or FALSE in case of error
 * (of course, FALSE can also be returned in non-error cases).
 * @err gives details in case of error
 */
gboolean     im_conf_get_bool    (ImConf* self, const gchar* key, GError **err);


/** 
 * im_conf_get_list:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @list_type: the type of the elements of the list
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a list of values from the configuration system
 *  
 * Returns: a list with the values for the key, or NULL in case of error
 * @err gives details in case of error
 */
GSList*     im_conf_get_list    (ImConf* self, const gchar* key, 
				 ImConfValueType list_type, GError **err);

/**
 * im_conf_set_string:
 * @self: a ImConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store a string value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     im_conf_set_string (ImConf* self, const gchar* key, const gchar *val,
				 GError **err);

/**
 * im_conf_set_int:
 * @self: a ImConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store an integer value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     im_conf_set_int    (ImConf* self, const gchar* key, int val,
				 GError **err);

/**
 * im_conf_set_float:
 * @self: a ImConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store an integer value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     im_conf_set_float  (ImConf* self, 
				 const gchar* key, 
				 gdouble val,
				 GError **err);

/**
 * im_conf_set_bool:
 * @self: a ImConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store a boolean value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     im_conf_set_bool    (ImConf* self, const gchar* key, gboolean val,
				  GError **err);


/** 
 * im_conf_set_list:
 * @self: a ImConf instance
 * @key: the key of the value to retrieve
 * @val: the list with the values to set
 * @list_type: the type of the elements of the list
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a list of values in the configuration system
 *
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     im_conf_set_list    (ImConf* self, const gchar* key, 
				  GSList *val, ImConfValueType list_type, 
				  GError **err);


/**
 * im_conf_list_subkeys:
 * @self: a ImConf instance
 * @key: the key whose subkeys will be listed
 * @err: a GError ptr, or NULL if not interested.
 *
 * list all the subkeys for a given key
 * 
 * Returns: a newly allocated list or NULL in case of error
 * the returned GSList must be freed by the caller
 * @err might give details in case of error
 */
GSList*     im_conf_list_subkeys    (ImConf* self, const gchar* key,
					GError **err);


/**
 * im_conf_remove_key:
 * @self: a ImConf instance
 * @key: the key to remove
 * @err: a GError ptr, or NULL if not interested.
 *
 * attempts to remove @key and all its subkeys
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err might give details in case of error
 */
gboolean   im_conf_remove_key    (ImConf* self, const gchar* key, GError **err);


/**
 * im_conf_key_exists:
 * @self: a ImConf instance
 * @key: the key to remove
 * @err: a GError ptr, or NULL if not interested.
 *
 * checks if the given key exists in the configuration system
 * 
 * Returns: TRUE if it exists, FALSE otherwise.
 * @err gives details in case of error
 */
gboolean   im_conf_key_exists   (ImConf* self, const gchar* key, GError **err);



/**
 * im_conf_key_valid:
 * @str: some key
 *
 * check whether @str is a valid key in the config system
 * This is a *class* function, and therefore does not require a ImConf
 * instance
 * 
 * Returns: TRUE if it is valid, FALSE otherwise
 */
gboolean im_conf_key_is_valid (const gchar* str);


/**
 * im_conf_key_escape:
 * @str: a non-empty string to escape
 *
 * returns an escaped version of @str, ie. something suitable as a key
 * This is a *class* function, and therefore does not require a ImConf
 * instance. Note: this for is invidual elements in a key
 * 
 * Returns: a newly allocated string with the escaped version
 */
gchar* im_conf_key_escape (const gchar* str);


/**
 * im_conf_key_escape:
 * @str: a string to escape
 *
 * returns an unescaped version of @str. This is a *class* function, and
 * therefore does not require a ImConf instance
 * Note: this for is invidual elements in a key
 * 
 * Returns: a newly allocated string with the unescaped version
 */
gchar* im_conf_key_unescape (const gchar* str);


void im_conf_listen_to_namespace (ImConf *self,
				  const gchar *namespace);

void im_conf_forget_namespace    (ImConf *self,
				  const gchar *namespace);
G_END_DECLS

#endif /* __IM_CONF_H__ */

