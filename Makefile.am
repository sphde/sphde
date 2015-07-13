## Process this file with autoconf to produce configure.

ACLOCAL_AMFLAGS = -I m4

include $(top_srcdir)/aminclude.am
man1_MANS = doc/man/man1/sasutil.1
$(man1_MANS): doc
EXTRA_DIST = $(man1_MANS) doc
MOSTLYCLEANFILES = $(DX_CLEANFILES)

dist_doc_DATA = COPYING ChangeLog.md

SUBDIRS = src
DIST_SUBDIRS = examples $(SUBDIRS)

ChangeLog.md:
	$(am__cd) $(top_srcdir) && ./generate-changelog.sh \
	  $(abs_top_builddir)/$@
