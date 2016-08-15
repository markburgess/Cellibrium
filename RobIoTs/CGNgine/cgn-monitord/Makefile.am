#
#  Copyright (C) CFEngine AS
#
#  This file is part of CFEngine 3 - written and maintained by CFEngine AS.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; version 3.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
#
# To the extent this program is licensed as part of the Enterprise
# versions of CFEngine, the applicable Commercial Open Source License
# (COSL) may apply to this file if you as a licensee so wish it. See
# included file COSL.txt.
#
noinst_LTLIBRARIES = libcgn-monitord.la

AM_CPPFLAGS = \
	$(OPENSSL_CPPFLAGS) \
	-I$(srcdir)/../libutils \
	-I$(srcdir)/../libcfnet \
	-I$(srcdir)/../libenv \
	-I$(srcdir)/../libpromises \
	$(PCRE_CPPFLAGS) \
	$(ENTERPRISE_CPPFLAGS)

AM_CFLAGS = @CFLAGS@ \
	$(OPENSSL_CFLAGS) \
	$(ENTERPRISE_CFLAGS)

libcgn_monitord_la_LIBADD = ../libpromises/libpromises.la

libcgn_monitord_la_SOURCES = \
        verify_measurements.c verify_measurements.h \
        env_monitor.c env_monitor.h \
        mon.h \
        mon_cpu.c \
        mon_disk.c \
        mon_entropy.c \
        mon_load.c \
        mon_network.c \
        mon_processes.c \
        mon_temp.c \
        cgn-monitord.c \
	history.c \
	history.h \
	mon_cumulative.c \
	mon_cumulative.h \
	monitoring.c \
	probes.c probes.h\
	env_monitor.c \
	env_monitor.h

if LINUX
libcgn_monitord_la_SOURCES += mon_io_linux.c
libcgn_monitord_la_SOURCES += mon_mem_linux.c
endif

if SOLARIS
libcgn_monitord_la_SOURCES += mon_mem_solaris.c
libcgn_monitord_la_SOURCES += shared_kstat.c shared_kstat.h
endif

if !LINUX
libcgn_monitord_la_SOURCES += mon_io_stub.c
if !SOLARIS
libcgn_monitord_la_SOURCES += mon_mem_stub.c
endif
endif


if !BUILTIN_EXTENSIONS
 sbin_PROGRAMS = cgn-monitord
 # Workaround for automake madness (try removing it if you want to know why).
 cgn_monitord_CFLAGS = $(AM_CFLAGS)
 cgn_monitord_LDADD = libcgn-monitord.la
endif

CLEANFILES = *.gcno *.gcda

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig *.rej
