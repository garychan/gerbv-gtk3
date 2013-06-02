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

# main program
bin_PROGRAMS = gerbv

gerbv_SOURCES = \
		opdefines.h \
		dynload.c dynload.h \
		attribute.c attribute.h \
                lrealpath.c lrealpath.h \
		main.h main.c \
		project.c project.h scheme-private.h scheme.c \
		scheme.h \
		callbacks.c callbacks.h interface.c \
		interface.h render.c render.h \
		icons.h \
		gimpruler.h \
		gimpruler.c

gerbv_LDADD = ../libgerbv/libgerbv.la
gerbv_DEPENDENCIES = ../libgerbv/libgerbv.la

# If we are building on win32, then compile in some icons for the
# desktop and application toolbar
if WIN32
gerbv_icon.o : gerbv_icon.ico $(srcdir)/gerbv.rc
	$(WINDRES) $(srcdir)/gerbv.rc $@

gerbv_icon.ico: $(top_srcdir)/desktop/gerbv_icon.ico
	cp $(top_srcdir)/desktop/gerbv_icon.ico $@

gerbv_LDADD+=		gerbv_icon.o
gerbv_DEPENDENCIES+=	gerbv_icon.o

endif

callbacks.$(OBJEXT) : authors.c bugs.c

TXT2CL=	sed -e 's;%;%%;g' -e 's;\\;\\\\;g' -e 's;";\\";g' -e 's;^;";g' -e 's;$$;",;g'

authors.c: $(top_srcdir)/AUTHORS Makefile
	if test -f $@.tmp ; then rm -f $@.tmp ; fi
	awk -F: '/^[ \t]*#/ {next}; /^[ \t]*$$/ {next}; {print $$1}' $(top_srcdir)/AUTHORS | ${TXT2CL} > $@.tmp
	num=`wc -l $@.tmp | awk '{print $$1 + 1}'` && echo "const gchar * authors_string_array[$$num] = {" > $@
	cat $@.tmp >> $@
	rm -f $@.tmp
	echo 'NULL};' >> $@

bugs.c: $(top_srcdir)/BUGS Makefile
	num=`wc -l $(top_srcdir)/BUGS | awk '{print $$1 + 1}'` && \
		echo "const gchar * bugs_string_array[$$num] = {" > $@
	${TXT2CL} $(top_srcdir)/BUGS >> $@
	echo 'NULL};' >> $@

CLEANFILES=	authors.c bugs.c

EXTRA_DIST=	gerbv.rc libgerbv.pc.in run_gerbv.in

SCMSUBDIR = scheme
scmdatadir = $(pkgdatadir)/${SCMSUBDIR}/
dist_scmdata_DATA = init.scm

uidir = $(pkgdatadir)/ui/
dist_ui_DATA = \
	modify-orientation-dialog.ui

AM_CFLAGS= -DBACKEND_DIR='"$(scmdatadir)"' -DSCMSUBDIR='"$(SCMSUBDIR)"' -DUIDIR='"$(pkgdatadir)/ui"'

all-local: run_gerbv
# create wrapper script that lets you test gerbv prior to installation
run_gerbv:     $(srcdir)/run_gerbv.in Makefile
	sed \
		-e 's;@SRCDIR@;${srcdir};g' \
		-e 's;@BUILDDIR@;${abs_builddir};g' \
		$(srcdir)/run_gerbv.in > $@
	chmod 755 $@

DISTCLEANFILES = run_gerbv

MOSTLYCLEANFILES = *~
MAINTAINERCLEANFILES = *~ *.o Makefile Makefile.in
