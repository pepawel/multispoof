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
sniff:
	sudo tcpdump -eni tap0
setup:
	sudo ip link set addr 55:55:55:55:55:55 dev tap0
	sudo killall dhclient-2.2.x
	sudo ip addr del 192.168.64.44/24 dev eth0
	sudo ip addr add 192.168.64.44/24 dev tap0
	sudo ip link set dev tap0 up
	sudo arp -i tap0 -s 192.168.64.200 00:00:00:00:00:02
	sudo ip route del default 2> /dev/null || true
	sudo ip route add default via 192.168.64.200
clean:
	rm -f *.o *.c~ *.h~ tx rx tapio netdb cmac test-netdb \
	cscope.out netdbsocket
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
