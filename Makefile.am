# $Id: Makefile.am,v 1.4 2016/04/08 23:04:04 sarrazip Exp $

SUBDIRS = src macros doc

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = \
	AUTHORS \
	COPYING \
	NEWS \
	README \
	THANKS

EXTRA_DIST = \
	config.rpath  \
	bootstrap \
	autogen.sh \
	mkinstalldirs

ACLOCAL_AMFLAGS = -I macros

MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	configure \
	config.guess \
	config.sub \
	depcomp \
	install-sh \
	missing
