# configure.ac :
#
# Authors:
#   Jose Dapena Paz <jdapena@igalia.com>
#
# Copyright (c) 2012, Igalia, S.L.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# * Neither the name of the Nokia Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

m4_define([iwkmail_version_major],[0])
m4_define([iwkmail_version_minor],[0])
m4_define([iwkmail_version_micro],[2])
m4_define([iwkmail_version],[iwkmail_version_major.iwkmail_version_minor.iwkmail_version_micro])

AC_INIT([Igalia WebKit Mail],[iwkmail_version],[http://www.igalia.com],[iwkmail])

AC_PREREQ([2.59])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AM_INIT_AUTOMAKE([1.9 foreign dist-xz no-dist-gzip tar-ustar])

# Use AM_SILENT_RULES if present
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE([enable])

# Initialize libtool
LT_PREREQ(2.2)
LT_INIT([dlopen disable-static])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

IT_PROG_INTLTOOL([0.50.0])

GLIB_GSETTINGS

PKG_PROG_PKG_CONFIG

GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

MORE_WARN_FLAGS="-Wdeclaration-after-statement"
DEPRECATION_FLAGS=

if test "$enable_maintainer_mode" = "yes"; then
	AC_DEFINE([MAINTAINER_MODE],[1],[Define to enable 'maintainer-only' behaviour])
	enable_debug=yes
	DEPRECATION_FLAGS="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
fi

GLIB_REQUIRED=2.32.0
GTK_REQUIRED=3.4.0
GCONF_REQUIRED=3.2.0
WEBKIT_GTK_REQUIRED=1.7.92
LIBSOUP_GNOME_REQUIRED=2.37.1
GSETTINGS_DESKTOP_SCHEMAS_REQUIRED=0.0.1
JSON_GLIB_REQUIRED=0.14.1
CAMEL_REQUIRED=3.4.0
GNOME_KEYRING_REQUIRED=3.4.0

PKG_CHECK_MODULES([DEPENDENCIES], [
		  glib-2.0 >= $GLIB_REQUIRED
		  gmodule-2.0
		  gthread-2.0
		  gio-unix-2.0 >= $GLIB_REQUIRED
		  gtk+-3.0 >= $GTK_REQUIRED
		  gtk+-unix-print-3.0 >= $GTK_REQUIRED
		  gconf-2.0 >= GCONF_REQUIRED
		  ice
		  x11
		  sm
		  webkitgtk-3.0 >= $WEBKIT_GTK_REQUIRED
		  libsoup-gnome-2.4 >= $LIBSOUP_GNOME_REQUIRED
		  gsettings-desktop-schemas >= $GSETTINGS_DESKTOP_SCHEMAS_REQUIRED
		  json-glib-1.0 >= $JSON_GLIB_REQUIRED
		  camel-1.2 >= $CAMEL_REQUIRED
		  gnome-keyring-1 >= $GNOME_KEYRING_REQUIRED
		  ])

# ******************
# Portability checks
# ******************

AC_CHECK_FUNCS([localtime_r memchr memmove memset mkdir mkdtemp realpath sqrt strchr strcspn strdup strerror strrchr strtol strtoul strstr])

# for backtrace()
AC_CHECK_HEADERS([execinfo.h fcntl.h])
AC_C_INLINE
AC_FUNC_MKTIME
AC_FUNC_STRTOD
AC_TYPE_SIZE_T

# *******************************
# Add warning flags
# *******************************

AM_CPPFLAGS="$AM_CPPFLAGS $DEPRECATION_FLAGS"
AM_CFLAGS="$AM_CFLAGS $WARN_CFLAGS $MORE_WARN_FLAGS"
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

# *******************************
# Internationalization
# ******************************* 

ISO_CODES_REQUIRED=0.35

PKG_CHECK_EXISTS([iso-codes >= $ISO_CODES_REQUIRED],
	[have_iso_codes=yes],[have_iso_codes=no])

if test "$have_iso_codes" = "yes"; then
	AC_MSG_CHECKING([whether iso-codes has iso-639 and iso-3166 domains])
	if $PKG_CONFIG --variable=domains iso-codes | grep -q 639 && \
	   $PKG_CONFIG --variable=domains iso-codes | grep -q 3166 ; then
		result=yes
	else
		result=no
		have_iso_codes=no
	fi
	AC_MSG_RESULT([$result])
fi

if test "$have_iso_codes" = "yes"; then
	AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX],["`$PKG_CONFIG --variable=prefix iso-codes`"],[ISO codes prefix])
	AC_DEFINE([HAVE_ISO_CODES],[1],[Define if you have the iso-codes package])
else
	AC_MSG_ERROR([iso-codes is required])
fi

GETTEXT_PACKAGE=iwkmail
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[Gettext package])
AM_GLIB_GNU_GETTEXT

# *****************
# API Documentation
# *****************

GNOME_DOC_INIT
GTK_DOC_CHECK([1.0])

# ************
# Output files
# ************

AC_DEFINE([IWK_COMPILATION],[1],[Compiling IWKMail])

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/html/Makefile
data/js/Makefile
data/css/Makefile
po/Makefile.in
])

AC_OUTPUT

# *************************************
# *************************************

echo "
Igalia WebKit Mail was configured with the following options:
        
	Source code location       : $srcdir
	Compiler                   : $CC
	Prefix                     : $prefix
	Extra debugging support    : $enable_debug
"
