TEST= src/tcptest.o mq.o
GOBJ= src/gps.o mq.o
GOBJS= src/gps2.o mq.o
POBJ= src/pub.o mq.o
SOBJ= sub.o mq.o
SSOBJ= sub1.o mq.o
JTEST= jsontest.o mq.o
CC=g++
DEBUG=-g -O0
CXXFLAGS=-Wall $(DEBUG) -std=c++11
LDLIBS= -lamqpcpp -lev -lpthread -lSimpleAmqpClient -lrabbitmq

all: test

test: $(TEST)
	$(CC) $(CXXFLAGS) $(TEST) -o bin/test $(LDLIBS)

pub: $(POBJ)
	$(CC) $(CXXFLAGS) $(POBJ) -o bin/pub $(LDLIBS)

gps: $(GOBJ)
	$(CC) $(CXXFLAGS) $(GOBJ) -o bin/gps $(LDLIBS)

gpss: $(GOBJS)
	$(CC) $(CXXFLAGS) $(GOBJS) -o bin/gps2 $(LDLIBS)

sub: $(SOBJ)
	$(CC) $(CXXFLAGS) $(SOBJ) -o sub $(LDLIBS)

sub1: $(SSOBJ)
	$(CC) $(CXXFLAGS) $(SSOBJ) -o sub1 $(LDLIBS)

jsontest: $(JTEST)
	$(CC) $(CXXFLAGS) $(JTEST) -o jtest $(LDLIBS)

main.o: pub.cpp
	$(CC) $(CXXFLAGS) -c pub.cpp $(LDLIBS)

mq.o: src/mq.cpp include/mq.h
	$(CC) $(CXXFLAGS) -c src/mq.cpp $(LDLIBS)

clean:
	rm -f src/*.o *.o bin/pub bin/gps
