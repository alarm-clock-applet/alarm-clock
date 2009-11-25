include $(top_srcdir)/Makefile.decl

SUBDIRS = src data

EXTRA_DIST += autogen.sh

dist-hook:
	bzr log > $(distdir)/ChangeLog
