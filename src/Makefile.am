NULL =

bin_PROGRAMS = iwkmail

iwkmail_headers = \
	im-account-mgr.h \
	im-account-mgr-helpers.h \
	im-account-mgr-priv.h \
	im-account-protocol.h \
	im-account-settings.h \
	im-conf.h \
	im-content-id-request.h \
	im-error.h \
	im-file-utils.h \
	im-js-backend.h \
	im-js-gobject-wrapper.h \
	im-js-utils.h \
	im-mail-ops.h \
	im-pair.h \
	im-protocol-registry.h \
	im-protocol.h \
	im-server-account-settings.h \
	im-service-mgr.h \
	im-soup-request.h \
	im-window.h \
	$(NULL)

iwkmail_SOURCES = \
	$(iwkmail_headers) \
	im-account-mgr.c \
	im-account-mgr-helpers.c \
	im-account-protocol.c \
	im-account-settings.c \
	im-conf.c \
	im-content-id-request.c \
	im-error.c \
	im-file-utils.c \
	im-js-backend.c \
	im-js-gobject-wrapper.c \
	im-js-utils.c \
	im-mail-ops.c \
	im-main.c \
	im-pair.c \
	im-protocol.c \
	im-protocol-registry.c \
	im-server-account-settings.c \
	im-service-mgr.c \
	im-soup-request.c \
	im-window.c \
	$(BUILT_SOURCES) \
	$(NULL)

iwkmail_CFLAGS = \
	$(DEPENDENCIES_CFLAGS) \
	$(WEBKIT_CFLAGS) \
	$(AM_CFLAGS) \
	-DHTMLDIR=\""$(datadir)/iwkmail/html/"\"

iwkmail_LDFLAGS = -dlopen self

iwkmail_LDADD = \
	$(DEPENDENCIES_LIBS) \
	$(LIBINTL)

BUILT_SOURCES = \
	im-enum-types.c \
	im-enum-types.h \
	$(NULL)

im-enum-types.h: im-enum-types.h.template $(iwkmail_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template im-enum-types.h.template $(iwkmail_headers)) > $@

im-enum-types.c: im-enum-types.c.template $(iwkmail_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template im-enum-types.c.template $(iwkmail_headers)) > $@

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
