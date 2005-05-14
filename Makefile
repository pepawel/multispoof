# Installation paths
USR_PREFIX=/tmp/usr/local
VAR_PREFIX=/tmp/var/local

COMPONENTS_DIR=${USR_PREFIX}/lib/multispoof
BIN_DIR=${USR_PREFIX}/sbin
CACHE_DIR=${VAR_PREFIX}/cache/multispoof
DOC_DIR=${USR_PREFIX}/share/doc/multispoof

CFLAGS+= -ggdb -Wall ${shell pkg-config --cflags glib-2.0}
LIBNET=-lnet
PCAP=-lpcap
GLIB=${shell pkg-config --libs glib-2.0}
COMPONENTS=tapio rx tx netdb cmac deta natman scanarp ndbexec conncheck

# Dirty hack to make changelog and README available in tarball.
# It will break on _darcs hierarchy change.
CHANGELOG_HACK=_darcs/current/Changelog
README_HACK=_darcs/current/README
INSTALL=install

CO_DIR_ESCAPED=${shell echo ${COMPONENTS_DIR} | sed -e "s/\//\\\\\\//g"}
CA_DIR_ESCAPED=${shell echo ${CACHE_DIR} | sed -e "s/\//\\\\\\//g"}

# Program files
all: multispoof ${COMPONENTS}
multispoof: multispoof.in
	sed -e 's/<COMPONENTS_DIR>/${CO_DIR_ESCAPED}/' < multispoof.in | \
	sed -e 's/<CACHE_DIR>/${CA_DIR_ESCAPED}/' > multispoof
	chmod +x multispoof
tapio: tapio.o printpkt.o getpkt.o
	${CC} ${LDFLAGS} $+ -o $@
rx: rx.o printpkt.o
	${CC} ${LDFLAGS} ${PCAP} $+ -o $@
tx: tx.o getpkt.o
	${CC} ${LDFLAGS} ${LIBNET} $+ -o $@
netdb: netdb.o netdb-op.o netdb-db.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
# FIXME: remove dependency on libnet from cmac
cmac: cmac.o getpkt.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
# FIXME: remove dependency on libnet from deta
deta: deta.o getpkt.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
# FIXME: remove dependency on libnet from natman
natman: natman.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
# FIXME: remove dependency on libnet from scanarp
scanarp: scanarp.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
# FIXME: remove dependency on libnet from conncheck
conncheck: conncheck.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
ndbexec: ndbexec.c ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
test-netdb: tests/test-netdb.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@

README: README.html
	lynx -dump $< > $@

install: all README
	${INSTALL} -m 755 -d ${COMPONENTS_DIR}
	${INSTALL} -m 755 -d ${BIN_DIR}
	${INSTALL} -m 755 -d ${DOC_DIR}
	${INSTALL} -m 755 -d ${CACHE_DIR}
	${INSTALL} -m 755 access-test ${COMPONENTS} ${COMPONENTS_DIR}
	${INSTALL} -m 755 multispoof ${BIN_DIR}
	${INSTALL} -m 644 README ${DOC_DIR}
uninstall:
	rm -rf ${COMPONENTS_DIR}
	rm -f ${BIN_DIR}/multispoof
	rm -rf ${CACHE_DIR}
	rm -rf ${DOC_DIR}
clean:
	rm -f *.o tests/*.o *.c~ *.h~ multispoof ${COMPONENTS} test-netdb \
	cscope.out netdbsocket README

# Developement targets
cs:
	cscope -bR
indent:
	# GNU indent style, but -nbad subsitituted with -bad,
	# and using -bli0 instead of -bli2
	indent *.c *.h -bad -bap -nbc -bbo -bl -bli0 -bls -ncdb -nce -cp1 \
	-cs -di2 -ndj -nfc1 -nfca -hnl -i2 -ip5 -lp -pcs -nprs -psl -saf \
	-sai -saw -nsc -nsob
dist: clean README
	# If file Changelog exists in repository - stop, because
	# otherwise it would be deleted
	test ! -e ${CHANGELOG_HACK}
	# Like above for README
	test ! -e ${README_HACK}
	# Create changelog, move README to repository
	darcs changes > ${CHANGELOG_HACK}
	mv README ${README_HACK}
	# Create release
	darcs dist --dist-name multispoof-`darcs chan | egrep "\ \ tagged" \
	| head -n 1 | cut -d " " -f 4`
	# Remove stuff from repository
	rm ${CHANGELOG_HACK} ${README_HACK}
