# FIXME: don't link unnessesary libraries
CFLAGS=-ggdb -Wall
LDFLAGS=-lnet -lpcap
VERSION=`darcs chan | egrep "\ \ tagged" | head -n 1 | cut -d " " -f 4`
NAME=multispoof-${VERSION}
all: tapio rx tx
tapio: tapio.o rx-printpkt.o tx-getpkt.o
rx: rx.o rx-printpkt.o
tx: tx.o tx-getpkt.o
cs:
	cscope -bR
setup:
	ip link set addr 1:2:3:4:5:6 dev tap0
	ip addr add 10.0.0.1/24 dev tap0
	ip link set dev tap0 up
	ip route add 10.0.1.0/24 dev tap0
	ip route add 10.0.2.0/24 dev tap0
	ip route add 10.0.3.0/24 dev tap0
	arp -i tap0 -s 10.0.0.2 a:b:c:d:e:f
clean:
	rm -f *.o tx rx tapio cscope.out
dist:
	test ! -e ${NAME}.tar.gz
	cd doc && make clean
	make clean
	darcs dist --dist-name ${NAME}
