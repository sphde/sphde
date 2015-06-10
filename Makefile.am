## Process this file with autoconf to produce configure.

ACLOCAL_AMFLAGS = -I m4

include $(top_srcdir)/aminclude.am
man1_MANS = doc/man/man1/sasutil.1
$(man1_MANS): doc
EXTRA_DIST = $(man1_MANS)
MOSTLYCLEANFILES = $(DX_CLEANFILES)

SUBDIRS = src
