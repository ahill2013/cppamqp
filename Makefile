TEST=src/proctest.o mq.o
VISTEST=src/visual_test.o mq.o
COMTEST=src/command_test.o mq.o
GOBJ= src/gps.o mq.o
GOBJS= src/gps2.o mq.o
JTEST= jsontest.o mq.o
CC=g++
DEBUG=-g -O0
CXXFLAGS=-Wall $(DEBUG) -std=c++11
LDLIBS= -lamqpcpp -lev -lpthread -lSimpleAmqpClient -lrabbitmq

all: gps

test: $(TEST)
	$(CC) $(CXXFLAGS) $(TEST) -o bin/test $(LDLIBS)

vis: $(VISTEST)
	$(CC) $(CXXFLAGS) $(VISTEST) -o bin/vis_test $(LDLIBS)

com: $(COMTEST)
	$(CC) $(CXXFLAGS) $(COMTEST) -o bin/com_test $(LDLIBS)

gps: $(GOBJ)
	$(CC) $(CXXFLAGS) $(GOBJ) -o bin/gps $(LDLIBS)

gpss: $(GOBJS)
	$(CC) $(CXXFLAGS) $(GOBJS) -o bin/gps2 $(LDLIBS)

jsontest: $(JTEST)
	$(CC) $(CXXFLAGS) $(JTEST) -o jtest $(LDLIBS)

main.o: pub.cpp
	$(CC) $(CXXFLAGS) -c pub.cpp $(LDLIBS)

mq.o: src/mq.cpp include/mq.h
	$(CC) $(CXXFLAGS) -c src/mq.cpp $(LDLIBS)

clean:
	rm -f src/*.o *.o bin/pub bin/gps bin/test bin/gps2
