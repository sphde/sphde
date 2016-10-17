## Process this file with autoconf to produce configure.

AM_CFLAGS   = -Wall -Werror -Wwrite-strings $(PTHREAD_CFLAGS)
AM_CXXFLAGS = -Wall -Werror -Wwrite-strings -Woverloaded-virtual \
	      $(PTHREAD_CFLAGS)

lib_LTLIBRARIES = libsphde.la libsphgtod.la libsphgettime.la
bin_PROGRAMS    = sasutil
EXTRA_DIST      =

# This rule is necessary because sphgtod_t needs to statically link
# .libs/libsphgtod.a and if make is executed in parallel (-jN) the sphgtod_t
# rule may be run before .libs/lisphgtod.a is built.
.libs/libsphgtod.a: libsphgtod.la

# This rule is necessary because sphgettime_t needs to statically link
# .libs/libsphgettime.a and if make is executed in parallel (-jN) the sphgettime_t
# rule may be run before .libs/lisphgettime.a is built.
.libs/libsphgettime.a: libsphgettime.la

# libsphde definitions
libsphdeincludedir = $(includedir)/sphde
libsphgtodincludedir = $(includedir)/sphde
libsphgettimeincludedir = $(includedir)/sphde

# The headers in 'libsphdeinclude_HEADERS' are meant to be the ones installed
# on 'make install'
libsphdeinclude_HEADERS = \
	sasatom.h \
	sasatom_generic.h \
	sasatom_i386.h \
	sasatom_powerpc.h \
	sasatom_x86_64.h \
	sascompoundheap.h \
	sphcompoundpcqheap.h \
	sasindexenum.h \
	sassim.h \
	sasalloc.h \
	sphcontext.h \
	saslock.h \
	sasmsync.h \
	sasindex.h \
	sasindexnode.h \
	sasindexkey.h \
	freenode.h \
	sasconf.h \
	sastype.h \
	sasstname.h \
	sassimpleheap.h \
	sassimplespace.h \
	sassimplestack.h \
	sasstringbtree.h \
	sasstringbtreeenum.h \
	sasstringbtreenode.h \
	sphsinglepcqueue.h \
	sphdirectpcqueue.h \
	sphlfentry.h \
	sphlflogentry.h \
	sphlflogger.h \
	sphlockfreeheap.h \
	sphlogportal.h \
	sphthread.h \
	sphtimer.h

libsphgtodinclude_HEADERS = sphgtod.h

libsphgettimeinclude_HEADERS = sphgettime.h

libsphde_la_INCLUDES = \
	$(libsphdeinclude_HEADERS) \
	bitv.h \
	sasiodef.h \
	sasulock.h \
	bitv_priv.h \
	sasio.h \
	sasshm.h \
	ultree.h \
	sasstringbtree.h \
	saslockl.h \
	sasanchr.h \
	sasindexnode.h \
	sasmlock.h \
	sasstringbtreenodepriv.h \
	sasindexnodepriv.h \
	sasstringbtreepriv.h \
	sasindexpriv.h \
	sasostrm.h \
	sasstdio.h 

libsphgtod_la_SOURCES = sphgtod.c

libsphgettime_la_SOURCES = sphgettime.c

libsphde_la_SOURCES = \
	sasshm.c \
	sasmsync.c \
	sasstname.c \
	sasio.c \
	bitv.c \
	freenode.c \
	sphtimer.c \
	sphthread.c \
	sassim.cpp \
	sasalloc.cpp \
	ultree.cpp \
	saslock.cpp \
	sasmlock.cpp \
	sasulock.cpp \
	sassimplespace.cpp \
	sassimplestack.cpp \
	sassimpleheap.cpp \
	sascompoundheap.cpp \
	sasindex.cpp \
	sasindexnode.cpp \
	sasindexenum.cpp \
	sasstringbtreenode.cpp \
	sasstringbtree.cpp \
	sasstringbtreeenum.cpp \
	sphcontext.cpp \
	sphlockfreeheap.cpp \
	sphlflogger.cpp \
	sphlogportal.cpp \
	sphsinglepcqueue.cpp \
	sasallocpriv.h \
	sphcontextpriv.h

if HTM
libsphdeinclude_HEADERS += sphmultipcqueue.h
libsphde_la_SOURCES += sphmultipcqueue.cpp
endif

libsphde_la_CFLAGS   = -D__SASSIM__ $(AM_CFLAGS)
libsphde_la_CXXFLAGS = -D__SASSIM__ $(AM_CXXFLAGS)

libsphgtod_la_CFLAGS   = $(AM_CFLAGS)
libsphgtod_la_CXXFLAGS = $(AM_CXXFLAGS)

# libsphgtod.a is ONLY to be a static archive in order to benefit from
# the performance aspects of inlining the emulated gettimeofday code.
libsphgtod_la_LDFLAGS = -all-static
libsphgtod_la_LIBADD = -lrt

libsphgettime_la_CFLAGS   = $(AM_CFLAGS)
libsphgettime_la_CXXFLAGS = $(AM_CXXFLAGS)

# libsphgettime.a is ONLY to be a static archive in order to benefit from
# the performance aspects of inlining the emulated clock_gettime code.
libsphgettime_la_LDFLAGS = -all-static
libsphgettime_la_LIBADD = -lrt

libsphde_la_LDFLAGS  = -version-info $(SPHDE_SO_VERSION)
libsphde_la_LIBADD   = $(PTHREAD_LIBS) -lrt

EXTRA_DIST          += $(libsphde_la_INCLUDES) saslockl.cpp


sasutil_SOURCES      = sasutil.c
sasutil_LDADD        = libsphde.la


# Test programs invoked with 'make check'
TESTS = 
TESTS            += sphgtod_t
sphgtod_t_SOURCES = tests/sphgtod_t.c
sphgtod_t_LDADD   = .libs/libsphgtod.a libsphde.la

TESTS            += sphgettime_t
sphgettime_t_SOURCES = tests/sphgettime_t.c
sphgettime_t_LDADD   = .libs/libsphgettime.a libsphde.la

TESTS            += sasatom_t
sasatom_t_SOURCES = tests/sasatom_t.c
sasatom_t_LDADD   = libsphde.la

TESTS           += bitvec_t
bitvec_t_SOURCES = tests/bitvec_t.c
bitvec_t_LDADD   = libsphde.la

TESTS            += sassim_t
sassim_t_SOURCES  = tests/sassim_t.c
sassim_t_LDADD    = libsphde.la

TESTS            += sascompoundheap_t
sascompoundheap_t_SOURCES  = tests/sascompoundheap_t.c
sascompoundheap_t_LDADD    = libsphde.la

TESTS            += sasseg_t
sasseg_t_SOURCES  = tests/sasseg_t.c
sasseg_t_LDADD    = libsphde.la

TESTS                    += sphlockfreeheap_t
sphlockfreeheap_t_SOURCES = tests/sphlockfreeheap_t.c
sphlockfreeheap_t_LDADD   = libsphde.la

TESTS                += sphlflogger_t
sphlflogger_t_SOURCES = tests/sphlflogger_t.c
sphlflogger_t_LDADD   = libsphde.la

TESTS              += sphthread_t
sphthread_t_SOURCES = tests/sphthread_t.c
sphthread_t_LDADD   = libsphde.la

TESTS                  += sphlflogger_tt
sphlflogger_tt_SOURCES  = tests/sphlflogger_tt.c
sphlflogger_tt_CFLAGS   = -DSPH_TIMERTEST_VERIFY=1 $(AM_CFLAGS)
sphlflogger_tt_LDADD    = libsphde.la

TESTS                   += sphlflogger_ttt
sphlflogger_ttt_SOURCES  = tests/sphlflogger_ttt.c
sphlflogger_ttt_CFLAGS   = -DSPH_TIMERTEST_VERIFY=1 $(AM_CFLAGS)
sphlflogger_ttt_LDADD    = libsphde.la

TESTS                 += sphlogportal_t
sphlogportal_t_SOURCES = tests/sphlogportal_t.c
sphlogportal_t_LDADD   = libsphde.la

TESTS                  += sphlogportal_tt
sphlogportal_tt_SOURCES = tests/sphlogportal_t.c
sphlogportal_tt_LDADD   = libsphde.la

TESTS                += sphcontext_t
sphcontext_t_SOURCES = tests/sphcontext_t.c
sphcontext_t_LDADD   = libsphde.la

TESTS                += sasindex_t
sasindex_t_SOURCES = tests/sasindex_t.c
sasindex_t_LDADD   = libsphde.la

TESTS                += sasindex_tt
sasindex_tt_SOURCES = tests/sasindex_tt.c
sasindex_tt_LDADD   = libsphde.la

TESTS                += sasstringbtree_t
sasstringbtree_t_SOURCES = tests/sasstringbtree_t.c
sasstringbtree_t_LDADD   = libsphde.la

TESTS                += sasstringbtree_tt
sasstringbtree_tt_SOURCES = tests/sasstringbtree_tt.c
sasstringbtree_tt_LDADD   = libsphde.la

TESTS                 += sphsinglepcqueue_t
sphsinglepcqueue_t_SOURCES = tests/sphsinglepcqueue_t.c
sphsinglepcqueue_t_LDADD   = libsphde.la

TESTS                 += sphsinglepcqueue_tt
sphsinglepcqueue_tt_SOURCES = tests/sphsinglepcqueue_tt.c
sphsinglepcqueue_tt_LDADD   = libsphde.la

TESTS                 += sphsinglepcqueue_ttt
sphsinglepcqueue_ttt_SOURCES = tests/sphsinglepcqueue_ttt.c
sphsinglepcqueue_ttt_LDADD   = libsphde.la

TESTS                 += sphdirectpcqueue_ttt
sphdirectpcqueue_ttt_SOURCES = tests/sphdirectpcqueue_ttt.c
sphdirectpcqueue_ttt_LDADD   = libsphde.la

if HTM
TESTS                 += sphmultipcqueue_t
sphmultipcqueue_t_SOURCES = tests/sphmultipcqueue_t.c
sphmultipcqueue_t_LDADD   = libsphde.la
endif

check_PROGRAMS = $(TESTS)
