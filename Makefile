TEST=src/test/proctest.o mq.o processor.o
MCTEST=src/mc/mc.o mq.o processor.o
VISTEST=src/test/visual_test.o mq.o processor.o
COMTEST=src/test/command_test.o mq.o processor.o
GOBJ= src/gps/gps.o mq.o processor.o
GOBJS= src/gps/gps2.o mq.o processor.o
JTEST= jsontest.o mq.o
CC=g++
DEBUG=-g -O0
CXXFLAGS=-Wall $(DEBUG) -std=c++11
LDLIBS= -lamqpcpp -lev -lpthread -lSimpleAmqpClient -lrabbitmq

all: gps test

test: $(TEST)
	$(CC) $(CXXFLAGS) $(TEST) -o bin/test $(LDLIBS)

vis: $(VISTEST)
	$(CC) $(CXXFLAGS) $(VISTEST) -o bin/vis_test $(LDLIBS)

com: $(COMTEST)
	$(CC) $(CXXFLAGS) $(COMTEST) -o bin/com_test $(LDLIBS)

mc: $(MCTEST)
	$(CC) $(CXXFLAGS) $(MCTEST) -o bin/mc $(LDLIBS)

gps: $(GOBJ)
	$(CC) $(CXXFLAGS) $(GOBJ) -o bin/gps $(LDLIBS)

gpss: $(GOBJS)
	$(CC) $(CXXFLAGS) $(GOBJS) -o bin/gps2 $(LDLIBS)

jsontest: $(JTEST)
	$(CC) $(CXXFLAGS) $(JTEST) -o bin/jtest $(LDLIBS)

mq.o: src/mq.cpp include/mq.h
	$(CC) $(CXXFLAGS) -c src/mq.cpp $(LDLIBS)

processor.o: src/processor.cpp include/processor.h
	$(CC) $(CXXFLAGS) -c src/processor.cpp $(LDLIBS)

clean:
	rm -f src/*.o src/mc/*.o src/gps/*.o src/test/*.o *.o bin/pub bin/gps bin/test bin/gps2
