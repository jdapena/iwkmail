
/* Generated data (by glib-mkenums) */

#include "im-enum-types.h"

/* enumerations from "im-account-settings.h" */
#include "im-account-settings.h"

GType
im_account_type_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ IM_ACCOUNT_TYPE_STORE,
			  "IM_ACCOUNT_TYPE_STORE",
			  "store" },
			{ IM_ACCOUNT_TYPE_TRANSPORT,
			  "IM_ACCOUNT_TYPE_TRANSPORT",
			  "transport" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ImAccountType"),
				values);
	}
	return the_type;
}

GType
im_account_retrieve_type_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ IM_ACCOUNT_RETRIEVE_HEADERS_ONLY,
			  "IM_ACCOUNT_RETRIEVE_HEADERS_ONLY",
			  "headers-only" },
			{ IM_ACCOUNT_RETRIEVE_MESSAGES,
			  "IM_ACCOUNT_RETRIEVE_MESSAGES",
			  "messages" },
			{ IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS,
			  "IM_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS",
			  "messages-and-attachments" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ImAccountRetrieveType"),
				values);
	}
	return the_type;
}

/* enumerations from "im-conf.h" */
#include "im-conf.h"

GType
im_conf_value_type_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ IM_CONF_VALUE_INT,
			  "IM_CONF_VALUE_INT",
			  "int" },
			{ IM_CONF_VALUE_BOOL,
			  "IM_CONF_VALUE_BOOL",
			  "bool" },
			{ IM_CONF_VALUE_FLOAT,
			  "IM_CONF_VALUE_FLOAT",
			  "float" },
			{ IM_CONF_VALUE_STRING,
			  "IM_CONF_VALUE_STRING",
			  "string" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ImConfValueType"),
				values);
	}
	return the_type;
}

GType
im_conf_event_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ IM_CONF_EVENT_KEY_CHANGED,
			  "IM_CONF_EVENT_KEY_CHANGED",
			  "changed" },
			{ IM_CONF_EVENT_KEY_UNSET,
			  "IM_CONF_EVENT_KEY_UNSET",
			  "unset" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ImConfEvent"),
				values);
	}
	return the_type;
}

/* enumerations from "im-error.h" */
#include "im-error.h"

GType
im_error_code_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ IM_ERROR_INTERNAL,
			  "IM_ERROR_INTERNAL",
			  "internal" },
			{ IM_ERROR_CONF_INVALID_VALUE,
			  "IM_ERROR_CONF_INVALID_VALUE",
			  "conf-invalid-value" },
			{ IM_ERROR_SOUP_INVALID_URI,
			  "IM_ERROR_SOUP_INVALID_URI",
			  "soup-invalid-uri" },
			{ IM_ERROR_ACCOUNT_MGR_ADD_ACCOUNT_FAILED,
			  "IM_ERROR_ACCOUNT_MGR_ADD_ACCOUNT_FAILED",
			  "account-mgr-add-account-failed" },
			{ IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED,
			  "IM_ERROR_ACCOUNT_MGR_DELETE_ACCOUNT_FAILED",
			  "account-mgr-delete-account-failed" },
			{ IM_ERROR_SETTINGS_INVALID_ACCOUNT_NAME,
			  "IM_ERROR_SETTINGS_INVALID_ACCOUNT_NAME",
			  "settings-invalid-account-name" },
			{ IM_ERROR_SETTINGS_INVALID_AUTH_PROTOCOL,
			  "IM_ERROR_SETTINGS_INVALID_AUTH_PROTOCOL",
			  "settings-invalid-auth-protocol" },
			{ IM_ERROR_SETTINGS_INVALID_CONNECTION_PROTOCOL,
			  "IM_ERROR_SETTINGS_INVALID_CONNECTION_PROTOCOL",
			  "settings-invalid-connection-protocol" },
			{ IM_ERROR_SETTINGS_INVALID_EMAIL_ADDRESS,
			  "IM_ERROR_SETTINGS_INVALID_EMAIL_ADDRESS",
			  "settings-invalid-email-address" },
			{ IM_ERROR_SETTINGS_INVALID_HOST,
			  "IM_ERROR_SETTINGS_INVALID_HOST",
			  "settings-invalid-host" },
			{ IM_ERROR_SETTINGS_INVALID_PROTOCOL,
			  "IM_ERROR_SETTINGS_INVALID_PROTOCOL",
			  "settings-invalid-protocol" },
			{ IM_ERROR_SETTINGS_INVALID_USERNAME,
			  "IM_ERROR_SETTINGS_INVALID_USERNAME",
			  "settings-invalid-username" },
			{ IM_ERROR_SEND_INVALID_PARAMETERS,
			  "IM_ERROR_SEND_INVALID_PARAMETERS",
			  "send-invalid-parameters" },
			{ IM_ERROR_SEND_NO_RECIPIENTS,
			  "IM_ERROR_SEND_NO_RECIPIENTS",
			  "send-no-recipients" },
			{ IM_ERROR_SEND_PARSING_RECIPIENTS,
			  "IM_ERROR_SEND_PARSING_RECIPIENTS",
			  "send-parsing-recipients" },
			{ IM_ERROR_SEND_INVALID_ACCOUNT_FROM,
			  "IM_ERROR_SEND_INVALID_ACCOUNT_FROM",
			  "send-invalid-account-from" },
			{ IM_ERROR_SEND_FAILED_TO_ADD_TO_OUTBOX,
			  "IM_ERROR_SEND_FAILED_TO_ADD_TO_OUTBOX",
			  "send-failed-to-add-to-outbox" },
			{ IM_ERROR_SEND_INVALID_ATTACHMENT,
			  "IM_ERROR_SEND_INVALID_ATTACHMENT",
			  "send-invalid-attachment" },
			{ IM_ERROR_COMPOSER_FAILED_TO_ADD_TO_DRAFTS,
			  "IM_ERROR_COMPOSER_FAILED_TO_ADD_TO_DRAFTS",
			  "composer-failed-to-add-to-drafts" },
			{ IM_ERROR_AUTH_FAILED,
			  "IM_ERROR_AUTH_FAILED",
			  "auth-failed" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ImErrorCode"),
				values);
	}
	return the_type;
}


/* Generated data ends here */

