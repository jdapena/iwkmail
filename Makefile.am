SUBDIRS = src po

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

NULL =

EXTRA_DIST = 			\
	autogen.sh		\
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-debug			\
	--enable-gtk-doc		\
	--disable-schemas-install

# Ignore gtk theme cache files on distcheck
distuninstallcheck_listfiles = find . -type f -print | grep -v 'icon-theme.cache'

# Build ChangeLog from git history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog
