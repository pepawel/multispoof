# FIXME: don't link unnessesary libraries
CFLAGS=-ggdb -Wall ${shell pkg-config --cflags glib-2.0}
LDFLAGS=-lnet -lpcap ${shell pkg-config --libs glib-2.0}
VERSION=`darcs chan | egrep "\ \ tagged" | head -n 1 | cut -d " " -f 4`
NAME=multispoof-${VERSION}
# Dirty hack to make changelog available in tarball
# It will break on _darcs hierarchy change.
CHANGELOG_HACK=_darcs/current/Changelog
all: tapio rx tx netdb cmac
tapio: tapio.o printpkt.o getpkt.o
rx: rx.o printpkt.o
tx: tx.o getpkt.o
netdb: netdb.o netdb-op.o netdb-db.o validate.o
cmac: cmac.o getpkt.o printpkt.o ndb-client.o validate.o
test-netdb: test-netdb.o ndb-client.o
cs:
	cscope -bR
defmac:
	echo "setvar defmac 55:55:55:55:55:55" | \
	socat UNIX-CONNECT:netdbsocket -
setup:
	sudo ip link set addr 1:2:3:4:5:6 dev tap0
	sudo ip addr add 10.0.0.1/24 dev tap0
	sudo ip link set dev tap0 up
	sudo ip route add 10.0.1.0/24 dev tap0
	sudo ip route add 10.0.2.0/24 dev tap0
	sudo ip route add 10.0.3.0/24 dev tap0
	sudo arp -i tap0 -s 10.0.0.2 a:b:c:d:e:f
clean:
	rm -f *.o tx rx tapio netdb cmac test-netdb cscope.out netdbsocket
dist:
	# Prevent overwriting already released tarball
	test ! -e ${NAME}.tar.gz
	# If file Changelog exists in repository - stop, because
	# otherwise it would be deleted
	test ! -e ${CHANGELOG_HACK}
	# Clean
	cd doc && make clean
	make clean
	# Create changelog, create release, delete changelog
	darcs changes > ${CHANGELOG_HACK}
	darcs dist --dist-name ${NAME}
	rm ${CHANGELOG_HACK}
