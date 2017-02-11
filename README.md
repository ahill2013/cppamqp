### Instructions for installing both libraries:

- Need to install boost: `sudo apt-get install libboost-all-dev`
- Need to install cmake: `sudo apt-get install cmake`
- Need to install doxygen: `sudo apt-get install doxygen`
- Need to install libev: `sudo apt-get install libev-dev`
- Need to install libssl-dev: `sudo apt-get install libssl-dev`

#### How to install rabbitmq-c:

Download rabbitmq-c from GitHub ([download](https://github.com/alanxz/rabbitmq-c)) 

In rabbitmq-c folder run these commands:
```bash
mkdir build && cd build
cmake -ENABLE_SSL_SUPPORT=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo cmake --build . --target install
```

After installing rabbitmq-c you need to change the PATH variable to include these two directories:
- /usr/local/lib (SimpleAmqpClient needs this)
- /usr/local/lib/x86_64-linux-gnu (rabbitmq-c needs this)

You also need to do this specifically *** **(IMPORTANT)** *** 
```bash
sudo nano /etc/ld.so.conf
```
- to this file, add the lines
 - /usr/local/lib
 - /usr/local/lib/x86_64-linux-gnu

#### To build and install SimpleAmqpClient 

Need to download it from GitHub ([download](https://github.com/alanxz/SimpleAmqpClient))

In the SimpleAmqpClient folder run these commands:
```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
sudo cmake --build . --target install
```

Run `sudo ldconfig`

#### AMQP-CPP
Install AMQP-CPP by downloading it from GitHub and running their install commands
([download](https://github.com/CopernicaMarketingSoftware/AMQP-CPP))

#Usage
bin/mc --ip "amqp://192.168.11.2" -m -n
