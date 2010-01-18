include $(top_srcdir)/Makefile.decl

SUBDIRS = src data po

EXTRA_DIST +=               \
    autogen.sh              \
    intltool-extract.in     \
    intltool-merge.in       \
    intltool-update.in

DISTCLEANFILES =                \
        intltool-extract        \
        intltool-merge          \
        intltool-update


dist-hook:
	bzr log > $(distdir)/ChangeLog
