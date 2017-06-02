#!/bin/bash

sudo cp -r lib/rapidjson /usr/include

g++ -std=c++11 -c -fPIC src/mq.cpp
g++ -std=c++11 -c -fPIC src/processor.cpp
g++ -std=c++11 -c -fPIC src/linearizer.cpp

g++ -shared -Wl,-soname,libCommFrame.so.1 -o libCommFrame.so.1.0 *.o

# sudo cp -r lib/rapidjson /usr/include
sudo cp libCommFrame.so.1.0 /usr/lib

sudo rm -r /usr/include/myamqp; sudo mkdir /usr/include/myamqp
sudo cp include/* /usr/include/myamqp

sudo ln -sf /usr/lib/libCommFrame.so.1.0 /usr/lib/libCommFrame.so.1
sudo ln -sf /usr/lib/libCommFrame.so.1 /usr/lib/libCommFrame.so

