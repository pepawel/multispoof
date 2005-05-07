CFLAGS+= -ggdb -Wall ${shell pkg-config --cflags glib-2.0}

LIBNET=-lnet
PCAP=-lpcap
GLIB=${shell pkg-config --libs glib-2.0}

VERSION=`darcs chan | egrep "\ \ tagged" | head -n 1 | cut -d " " -f 4`
NAME=multispoof-${VERSION}
# Dirty hack to make changelog available in tarball
# It will break on _darcs hierarchy change.
CHANGELOG_HACK=_darcs/current/Changelog

# Program files
all: tapio rx tx netdb cmac deta natman
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
deta: deta.o getpkt.o printpkt.o ndb-client.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
# FIXME: remove dependency on libnet from natman
natman: natman.o ndb-client.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
test-netdb: test-netdb.o ndb-client.o
	${CC} ${LDFLAGS} ${GLIB} ${LIBNET} $+ -o $@
	
clean:
	rm -f *.o *.c~ *.h~ tx rx tapio netdb cmac test-netdb \
	deta natman cscope.out netdbsocket

# Developement targets
cs:
	cscope -bR
lb:
	sudo iptables -t nat -F
	sudo iptables -t nat -X
	sudo iptables -t nat -A POSTROUTING -o tap0 -m nth --every 2 \
	--packet 0 -j SNAT --to-source 192.168.64.44
	sudo iptables -t nat -A POSTROUTING -o tap0 -m nth --every 2 \
	--packet 1 -j SNAT --to-source 192.168.64.3
indent:
	# GNU indent style, but -nbad subsitituted with -bad,
	# and using -bli0 instead of -bli2
	indent *.c *.h -bad -bap -nbc -bbo -bl -bli0 -bls -ncdb -nce -cp1 \
	-cs -di2 -ndj -nfc1 -nfca -hnl -i2 -ip5 -lp -pcs -nprs -psl -saf \
	-sai -saw -nsc -nsob
dist:
	# Prevent overwriting already released tarball
	test ! -e ${NAME}.tar.gz
	# If file Changelog exists in repository - stop, because
	# otherwise it would be deleted
	test ! -e ${CHANGELOG_HACK}
	# Clean
	make clean
	# Create changelog, create release, delete changelog
	darcs changes > ${CHANGELOG_HACK}
	darcs dist --dist-name ${NAME}
	rm ${CHANGELOG_HACK}
