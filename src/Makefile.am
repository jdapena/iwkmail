NULL =

bin_PROGRAMS = iwkmail

iwkmail_SOURCES = \
	im-main.c

iwkmail_CFLAGS = \
	$(DEPENDENCIES_CFLAGS) \
	$(WEBKIT_CFLAGS) \
	$(AM_CFLAGS)

iwkmail_LDFLAGS = -dlopen self

iwkmail_LDADD = \
	$(DEPENDENCIES_LIBS) \
	$(LIBINTL)

