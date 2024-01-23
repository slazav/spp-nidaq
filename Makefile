srcdir = spp-nidaq

all:
	make -C ${srcdir}


DESTDIR    ?=
prefix     ?= $(DESTDIR)/usr
bindir     ?= $(prefix)/bin

install: all
	nstall -D -m755 ${srcdir}/spp-nidaq -t ${bindir}
