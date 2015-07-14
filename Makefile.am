## Process this file with autoconf to produce configure.

ACLOCAL_AMFLAGS = -I m4

include $(top_srcdir)/aminclude.am

dist_doc_DATA = COPYING ChangeLog.md

SUBDIRS = src
DIST_SUBDIRS = examples $(SUBDIRS)

man1_MANS = doc/man/man1/sasutil.1
EXTRA_DIST = $(man1_MANS) doc

$(man1_MANS): doc

MOSTLYCLEANFILES = $(DX_CLEANFILES)

distclean-local: distclean-local-doc
.PHONY: distclean-local-doc
distclean-local-doc:
	-rm -rf doc/latex doc/doxygen_sqlite3.db

ChangeLog.md:
	$(am__cd) $(top_srcdir) && ./generate-changelog.sh > \
	  $(abs_top_builddir)/$@
