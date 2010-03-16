include $(top_srcdir)/Makefile.decl

SUBDIRS = src data po

EXTRA_DIST +=               \
    autogen.sh              \
    intltool-extract.in     \
    intltool-merge.in       \
    intltool-update.in

DISTCLEANFILES =            \
    intltool-extract        \
    intltool-merge          \
    intltool-update


dist-hook: gen-ChangeLog

.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d .bzr; then                           \
      bzr log > $(distdir)/cl-t;                	\
      rm -f $(distdir)/ChangeLog;                   \
      mv $(distdir)/cl-t $(distdir)/ChangeLog;      \
    fi
