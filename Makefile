# FIXME: don't link unnessesary libraries
# "+= " make it possible to specify additional parameters:
# LDLAGS=-static make
CFLAGS+= -ggdb -Wall ${shell pkg-config --cflags glib-2.0}
LDFLAGS+= -lnet -lpcap ${shell pkg-config --libs glib-2.0}
VERSION=`darcs chan | egrep "\ \ tagged" | head -n 1 | cut -d " " -f 4`
NAME=multispoof-${VERSION}
# Dirty hack to make changelog available in tarball
# It will break on _darcs hierarchy change.
CHANGELOG_HACK=_darcs/current/Changelog

# Program files
all: tapio rx tx netdb cmac arprep
tapio: tapio.o printpkt.o getpkt.o
rx: rx.o printpkt.o
tx: tx.o getpkt.o
netdb: netdb.o netdb-op.o netdb-db.o validate.o
cmac: cmac.o getpkt.o printpkt.o ndb-client.o validate.o
arprep: arprep.o getpkt.o printpkt.o ndb-client.o
test-netdb: test-netdb.o ndb-client.o
	
clean:
	rm -f *.o *.c~ *.h~ tx rx tapio netdb cmac test-netdb \
	arprep cscope.out netdbsocket

# Developement targets
cs:
	cscope -bR
defmac:
	echo "setvar defmac `ip link|grep -A 1 tap0|tail -n 1 \
	      | cut -d ' ' -f 6`" | socat UNIX-CONNECT:netdbsocket -
setup:
	sudo killall dhclient || true
	sudo ip addr del 192.168.64.44/24 dev eth0 || true
	sudo ip addr add 192.168.64.44/24 dev tap0 || true
	sudo ip link set dev tap0 up || true
	sudo arp -i tap0 -s 192.168.64.200 00:00:00:00:00:02 || true
	sudo ip route del default 2> /dev/null || true
	sudo ip route add default via 192.168.64.200 || true
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
