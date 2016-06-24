# Copyright (c) 2015 ilbers GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(mango-linux-apps, 1.0, asmirnov@ilbers.de)

AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile apps/Makefile apps/watchdog/Makefile apps/tcp_client/Makefile apps/hello_dc/Makefile apps/mangofb/Makefile)

AC_CONFIG_SRCDIR([apps/watchdog/wd_daemon.c])
AC_CONFIG_SRCDIR([apps/tcp_client/tcp_client.c])
AC_CONFIG_SRCDIR([apps/hello_dc/hello_dc.c])
AC_CONFIG_SRCDIR([apps/mangofb/mangofb.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([X11],[XSetForeground], [], AC_MSG_ERROR([Could  not find X11 library]))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_FORK

AC_OUTPUT
