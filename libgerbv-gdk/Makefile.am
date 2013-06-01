## -*- makefile -*-
## Process this file with automake to produce Makefile.in
##
##  gEDA - GNU Electronic Design Automation
##  This file is a part of gerbv.
##
##    Copyright (C) 2000-2003 Stefan Petersen (spe@stacken.kth.se)
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA

AM_CPPFLAGS = -I$(top_srcdir)/libgerbv

# shared library
lib_LTLIBRARIES = libgerbv-gdk.la

# public header file(s)
pkgincludedir=	$(includedir)/${PACKAGE}-${VERSION}
pkginclude_HEADERS= gerbv-gdk.h

# pkg-config data file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgerbv-gdk.pc

libgerbv_gdk_la_SOURCES= \
		gerbv-gdk.c \
		gerbv-gdk.h \
		draw-gdk.c \
		draw-gdk.h

# libgerbv shared library version.  A fairly strict rule should
# be followed for when this version is updated.  Those rules,
# taken from the libtool manual, are as follows
#
# - Format is current:revision:age
#
# 1. Start with version information of `0:0:0' for each libtool library.
#
# 2. Update the version information only immediately before a public
#    release of your software. More frequent updates are unnecessary,
#    and only guarantee that the current interface number gets larger faster.
#
# 3. If the library source code has changed at all since the last update,
#    then increment revision (`c:r:a' becomes `c:r+1:a').
#
# 4. If any interfaces have been added, removed, or changed since the last
#    update, increment current, and set revision to 0.
#
# 5. If any interfaces have been added since the last public release, then
#    increment age.
#
# 6. If any interfaces have been removed since the last public release, then
#    set age to 0.
#
libgerbv_gdk_la_LDFLAGS = -version-info 0:0:0 -no-undefined

EXTRA_DIST=	libgerbv-gdk.pc.in

MOSTLYCLEANFILES = *~
MAINTAINERCLEANFILES = *~ *.o Makefile Makefile.in
