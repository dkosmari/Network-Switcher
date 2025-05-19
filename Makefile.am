EXTRA_DIST = \
	assets \
	AUTHORS \
	bootstrap \
	COPYING \
	docker-build.sh \
	Dockerfile \
	README.md


SUBDIRS = external/libwupsxx


AM_CPPFLAGS = -I$(srcdir)/external/libwupsxx/include


AM_CXXFLAGS = -Wall -Wextra -Werror

LDADD = \
	external/libwupsxx/src/libwupsxx.a \
	-lnotifications


WPS_FILE = network-switcher.wps

noinst_PROGRAMS = network-switcher.elf


network_switcher_elf_SOURCES = \
	src/main.cpp


all-local: $(WPS_FILE)

.PHONY: run
run: $(WPS_FILE)
	WIILOAD=tcp:wiiu wiiload $<


install-exec-local: all
	curl --upload-file $(WPS_FILE) "ftp://wiiu:/fs/vol/external01/wiiu/environments/aroma/plugins/"

uninstall-local:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/environments/aroma/plugins/$(WPS_FILE)"


@INC_AMINCLUDE@

CLEANFILES = $(WPS_FILE)
DISTCLEANFILES = $(AMINCLUDE)


.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > $(srcdir)/compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> $(srcdir)/compile_flags.txt
