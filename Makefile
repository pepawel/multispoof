# Installation paths
USR_PREFIX=/usr/local
VAR_PREFIX=/var/local

COMPONENTS_DIR=${USR_PREFIX}/lib/multispoof
BIN_DIR=${USR_PREFIX}/sbin
CACHE_DIR=${VAR_PREFIX}/cache/multispoof
DOC_DIR=${USR_PREFIX}/share/doc/multispoof

CFLAGS+= -ggdb -Wall ${shell pkg-config --cflags glib-2.0}
ifndef PCAP_STATIC
	PCAP=-lpcap
endif
GLIB=${shell pkg-config --libs glib-2.0}
COMPONENTS=tapio rx tx netdb cmac deta natman scanarp ndbexec conncheck

# Dirty hack to make changelog, README and VERSION available in tarball.
# It will break on _darcs hierarchy change.
CHANGELOG_NAME=Changelog
CHANGELOG_HACK=_darcs/current/${CHANGELOG_NAME}
README_HACK=_darcs/current/README
VERSION_HACK=_darcs/current/VERSION
INSTALL=install

CO_DIR_ESCAPED=${shell echo ${COMPONENTS_DIR} | sed -e "s/\//\\\\\\//g"}
CA_DIR_ESCAPED=${shell echo ${CACHE_DIR} | sed -e "s/\//\\\\\\//g"}

# Program files
all: multispoof multispoof-dump ${COMPONENTS} README
multispoof: multispoof.in VERSION Makefile
	sed -e 's/<COMPONENTS_DIR>/${CO_DIR_ESCAPED}/' < multispoof.in | \
	sed -e 's/<CACHE_DIR>/${CA_DIR_ESCAPED}/' | \
	sed -e 's/<VERSION>/${shell cat VERSION}/' > multispoof
	chmod +x multispoof
tapio: tapio.o printpkt.o getpkt.o
	${CC} ${LDFLAGS} $+ -o $@
rx: rx.o printpkt.o ${PCAP_STATIC}
	${CC} ${LDFLAGS} ${PCAP} $+ -o $@
tx: tx.o getpkt.o ${PCAP_STATIC}
	${CC} ${LDFLAGS} ${PCAP} $+ -o $@
netdb: netdb.o netdb-op.o netdb-db.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
cmac: cmac.o getpkt.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
deta: deta.o getpkt.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
natman: natman.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
scanarp: scanarp.o printpkt.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
conncheck: conncheck.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
ndbexec: ndbexec.c ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@

test-netdb: tests/test-netdb.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@
multispoof-dump: multispoof-dump.o ndb-client.o validate.o
	${CC} ${LDFLAGS} ${GLIB} $+ -o $@

install: all
	${INSTALL} -m 755 -d ${COMPONENTS_DIR}
	${INSTALL} -m 755 -d ${BIN_DIR}
	${INSTALL} -m 755 -d ${DOC_DIR}
	${INSTALL} -m 755 -d ${CACHE_DIR}
	${INSTALL} -m 755 access-test ${COMPONENTS} ${COMPONENTS_DIR}
	${INSTALL} -m 755 multispoof ${BIN_DIR}
	${INSTALL} -m 755 multispoof-dump ${BIN_DIR}
	${INSTALL} -m 644 README ${DOC_DIR}
uninstall:
	rm -rf ${COMPONENTS_DIR}
	rm -f ${BIN_DIR}/multispoof
	rm -rf ${CACHE_DIR}
	rm -rf ${DOC_DIR}
clean:
	rm -f *.o tests/*.o *.c~ *.h~ multispoof ${COMPONENTS} \
		test-netdb multispoof-dump
# Developement targets
README: README.html
	lynx -dump $< > $@
VERSION:
	echo -n `darcs changes | egrep "\ \ tagged" | head -n 1 \
	| cut -d " " -f 4`	> $@
	darcs changes --last 1|head -n 2|tail -n 1|egrep -q "\ \ tagged" \
	|| echo -n plus`date +%Y%m%d` >> $@
	echo "" >> $@
indent:
	# GNU indent style, but -nbad subsitituted with -bad,
	# and using -bli0 instead of -bli2
	indent *.c *.h -bad -bap -nbc -bbo -bl -bli0 -bls -ncdb -nce -cp1 \
	-cs -di2 -ndj -nfc1 -nfca -hnl -i2 -ip5 -lp -pcs -nprs -psl -saf \
	-sai -saw -nsc -nsob
upload:
	# Create changelog
	darcs changes > ${CHANGELOG_NAME}
	# Uploads Changelog and README to sourceforge
	scp ${CHANGELOG_NAME} README.html \
		c09@shell.sourceforge.net:/home/groups/m/mu/multispoof/htdocs/
	rm ${CHANGELOG_NAME}
dist: clean README VERSION
	# If file Changelog exists in repository - stop, because
	# otherwise it would be deleted
	test ! -e ${CHANGELOG_HACK}
	# Like above for README and VERSION
	test ! -e ${README_HACK}
	test ! -e ${VERSION_HACK}
	# Create changelog, copy README and VERSION to repository
	darcs changes > ${CHANGELOG_HACK}
	cp README ${README_HACK}
	cp VERSION ${VERSION_HACK}
	# Create release
	darcs dist --dist-name multispoof-`cat VERSION`
	# Remove stuff from repository
	rm ${CHANGELOG_HACK} ${README_HACK} ${VERSION_HACK}
	# Sign tarball
	gpg -ab multispoof-`cat VERSION`.tar.gz || true
distclean: clean
	rm -f README VERSION
