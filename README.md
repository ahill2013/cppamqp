Instructions for installing both libraries:


SimpleAmqpClient:
    Need to install rabbitmq-c (download from github)
    Need to install boost (sudo apt-get install libboost-all-dev)
    Need to install cmake (sudo apt-get install cmake)
    Need to install doxygen (sudo apt-get install doxygen)
    Need to install libev (ev.h)

    How to install rabbitmq-c:

        Download rabbitmq-c from git
        In rabbitmq-c folder run these commands:
            mkdir build && cd build
            cmake -ENABLE_SSL_SUPPORT=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
            sudo cmake --build . --target install

    After installing rabbitmq-c you need to change the PATH variable to include these two directories
        * /usr/local/lib (SimpleAmqpClient needs this)
        * /usr/local/lib/x86_64-linux-gnu (rabbitmq-c needs this)

    You also need to do this specifically **** (IMPORTANT)
        * sudo nano /etc/ld.so.conf
            -- to this file add the lines
                /usr/local/lib
                /usr/local/lib/x86_64-linux-gnu

    To build and install SimpleAmqpClient
        Need to download it from github and in the SimpleAmqpClient folder run these commands:
            mkdir build && cd build
            cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
            sudo cmake --build . --target install

    Run sudo ldconfig 

    Install AMQP-CPP by downloading it from the repo and running their install commands
