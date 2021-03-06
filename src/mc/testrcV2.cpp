#include "../../include/mq.h"
#include "../../include/processor.h"
#include <sys/time.h>
#include <sys/types.h>
#include <tclap/CmdLine.h>

#include <stdio.h>     //libraries needed for UART
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <errno.h>


#define  SYNC     27
#define  SYNC2	  17
#define  SYNC3   22

//rc stuff needed and some global variables
#define estop 18
#define channelA 23
#define channelB 24
int centa, centb;

struct RCInfo {
    bool f = false;
    double v = 0;
    double w = 0;

    std::mutex linear;
    std::mutex angular;
    std::mutex flag;
} rcInfo;

void setLinear(double v) {
    rcInfo.linear.lock();
    rcInfo.v = v;
    rcInfo.linear.unlock();
}

double getLinear() {
    rcInfo.linear.lock();
    double v = rcInfo.v;
    rcInfo.linear.unlock();
    return v;
}

void setAngular(double w) {
    rcInfo.angular.lock();
    rcInfo.w = w;
    rcInfo.angular.unlock();
}

double getAngular() {
    rcInfo.angular.lock();
    double w = rcInfo.w;
    rcInfo.angular.unlock();
    return w;
}

void setFlag(bool f) {
    rcInfo.flag.lock();
    rcInfo.f = f;
    rcInfo.flag.unlock();
}

bool getFlag() {
    rcInfo.flag.lock();
    bool f = rcInfo.f;
    rcInfo.flag.unlock();
    return f;
}

MessageHeaders headers;
ExchKeys exchKeys;

int status = 30;
std::map<std::string, std::string> exchange_keys;

struct MotorPublishInfo {
    std::string exchange1 = exchKeys.gps_exchange;
    std::string exchange2 = exchKeys.nav_exchange;
    std::string exchange3 = exchKeys.control_exchange;
    std::string key = exchKeys.mc_key;
    std::string header = headers.WMBROAD;
} motorInfo;

struct StatusPublish {
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.control_key;
    std::string header = headers.WSTATUS;
} statusInfo;

// Mutexes to protect global data


struct Data {
    bool running = true;
    Command* command;
    Commands* commands = nullptr;
    GPSMessage* gpsMessage;
    bool updateGPS;
} _data;

struct Mutexes {
    std::mutex gps;
    std::mutex commands;
    std::mutex running;
    std::mutex update;
} _mutexes;

////sets a pin up to be used as a rc signal reader
void setPWMpin(int pin){
	pinMode (pin, INPUT);
	pullUpDnControl(pin, PUD_DOWN);
}

////reads the rc servo signal coming to the pin
int readPWM(int pin) {
	//waits for and catches peak start
		while(digitalRead(pin) == 0) {}
		int edgeA = micros();

		//waits for and catches peak end
		while(digitalRead(pin) == 1) {}
		int edgeB = micros();

		//calculates the length of the peak and returns it
		return edgeB-edgeA;
}

void rcControl(){
	int stop, chA, chB;  //peak time in microseconds
	double vmax = 1.0, wmax = 1.0;
    double w, v;

	
	stop = readPWM(estop);
	chA = readPWM(channelA);
	chB = readPWM(channelB);
	//std::cout << chA << " " << chB << std::endl;
	//constants for equation motions will change
	w =wmax* (chA-centa)/400;
	v = vmax*(chB-centb)/400;
	if(w < .015 && w > -.015){w = 0;}
	if(v<.015 && v >-.015){v = 0;}

//    v = 1.1;
//    w = 0.25;

//    stop = 1800;
    setLinear(v);
    setAngular(w);
		
	//if e stop is activated
	if(stop > 1700) {
		setFlag(true);
	}
	//e stop not used
	else {
		setFlag(false);
	}
}

void setRunning(bool value) {
    _mutexes.running.lock();
    _data.running = value;
    _mutexes.running.unlock();
}


bool getRunning() {
    _mutexes.running.lock();
    bool running = _data.running;
    _mutexes.running.unlock();
    return running;
}


void setCommands(Commands* commands) {
    _mutexes.commands.lock();

    if (_data.commands != nullptr) {
        delete _data.commands;
    }

    _data.commands = commands;
    _mutexes.commands.unlock();
}

Commands* getCommands() {
    _mutexes.commands.lock();
    Commands* commands = _data.commands;
    _mutexes.commands.unlock();
    return commands;
}

void setUpdate() {
    _mutexes.update.lock();
    _data.updateGPS = true;
    _mutexes.update.unlock();
}

bool getUpdate() {
    _mutexes.update.lock();
    bool update = _data.updateGPS;
    _mutexes.update.unlock();
    return update;
}

void setGPS(GPSMessage* gpsMessage) {
    _mutexes.gps.lock();

    if (_data.gpsMessage != nullptr) {
        delete gpsMessage;
    }

    _data.gpsMessage = gpsMessage;

    setUpdate();
    _mutexes.gps.unlock();
}

GPSMessage* getGPS() {
    _mutexes.gps.lock();
    GPSMessage* gpsMessage = _data.gpsMessage;
    _mutexes.gps.unlock();
    return gpsMessage;
}

//void interupt1(){
//
//    std::cout << "Attempting to Send" << std::endl;
//
//
//    _mutexes.commands.lock();
//
//    Command* toSend = nullptr;
//    if (!getFlag) {
//
//        if (_data.commands != nullptr) {
//            std::cout << "Pulling command" << std::endl;
//            toSend = _data.commands->remove();
//        }
//
//        if (toSend == nullptr) {
//            std::cout << "Using default command" << std::endl;
//            toSend = _data.command;
//        }
//    } else {
//        toSend = new Command(getLinear(), getAngular(), 0, 0, 0, 1);
//    }
//    std::cout << toSend->angvel << std::endl;
//
//    _mutexes.commands.unlock();
//
//    int uart0_filestream;
//    uart0_filestream = open("/dev/serial0", O_RDWR  | O_NOCTTY | O_NDELAY);
//    char *  p_tx_buffer;
//    char tx_buffer[20];
//    char converted[50];
//
//    std::cout << toSend->linvel << std::endl;
//    std::cout << toSend->angvel << std::endl;
//
//
//    double n = sprintf(converted, "%.2f %.2f",toSend->linvel,toSend->angvel);
//
//    p_tx_buffer=&tx_buffer[0];
//    std::cout<<"sending"<< std::endl;
//    if (uart0_filestream == -1){
//        std::cout <<"Error - Unable to open Uart. Ensure it is not in use by another application and that it is \n"<< std::endl;
//    }
//
//    //bellow I am setting up the UART Parameters
//    struct termios options;
//    tcgetattr(uart0_filestream, &options);
//    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
//    options.c_iflag = IGNPAR;
//    options.c_oflag = 0;
//    options.c_lflag = 0;
//    tcflush(uart0_filestream, TCIFLUSH);
//    tcsetattr(uart0_filestream, TCSANOW, &options);
//    *p_tx_buffer++=converted[0];
//    *p_tx_buffer++=converted[1];
//    *p_tx_buffer++=converted[2];
//    *p_tx_buffer++=converted[3];
//    *p_tx_buffer++=converted[4];
//    *p_tx_buffer++=converted[5];
//    *p_tx_buffer++=converted[6];
//    *p_tx_buffer++=converted[7];
//    *p_tx_buffer++=converted[8];
//    *p_tx_buffer++=converted[9];
//    *p_tx_buffer++=converted[10];
//    if(uart0_filestream !=-1){
//        int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer- &tx_buffer[0]));
//        //cout<<count<<endl;
//        if(count<0){
//            std::cout <<"UART TX error"<< std::endl;
//        }
//    }
//
//
//}


struct ev_loop* sub_loop = ev_loop_new();
std::mutex start;

/**
 * This is where all GPS computations will take place. Check data fields for changes on loop. If state of the
 * robot has changed respond accordingly.
 *
 * Not sure what is supposed to go here for mc_publisher
 *
 * Set up a loop to publish the location, acceleration, velocity, etc. once every tenth of a second.
 * TODO: Periodically send a status update to the Control unit
 * @param host string name of the host with the ip:port number as necessary
 */

// Listen for incoming information like commands from Control or requests from other components
void mc_subscriber(std::string host) {
//    start.lock();
    MessageHeaders headers1;

    std::string queue = exchKeys.mc_sub;

    MQSub* subscriber = new MQSub(*sub_loop, host, queue);
    AMQP::TcpChannel* chan = subscriber->getChannel();

    for (auto const& kv : exchKeys.declared) {
        setup_cop_exchange(chan, kv.first, kv.second);
    }


    auto startCb = [](const std::string& consumertag) {
        std::cout << "Consumer operation started" << std::endl;
    };

    auto errorCb = [](const char *message) {
        std::cout << message << std::endl;
        std::cout << "Consume operation failed" << std::endl;
    };

    // Handle commands. Every time a message arrives this is called
    auto messageCb = [chan, headers1](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "Message Delivered" << std::endl;

        // Do not remove this ever
        chan->ack(deliveryTag);
        std::string header = message.headers().get("MESSAGE");

        std::cout << header.c_str() << std::endl;

        if (header == headers1.WCOMMANDS) {
            std::cout << message.message() << std::endl;
            Commands* commands = Processor::decode_commands(message.message());
            setCommands(commands);
            std::cout << "Got commands" << std::endl;


        } else if (header == headers1.WGPSFRAME) {
            GPSMessage* gpsMessage = Processor::decode_gps(message.message(), true);
            setGPS(gpsMessage);
            // Code to send GPS?
        } else if (header == headers1.WCLOSE) {
            setRunning(false);
            std::cout << "Supposed to close" << std::endl;
        } else if (header == headers1.WSTART) {
  //          start.unlock();
        }
    };

    // Must do this for each set of exchanges and queues that are being listened to


    for (auto const & kv : exchange_keys) {
        setup_consumer(chan, subscriber->getQueue(), kv.first, kv.second);
    }

    std::cout << "Working now" << std::endl;

    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);    // Start consuming messages

    ev_run(sub_loop, 0);    // Run event loop ev_unloop(sub_loop) will kill the event loop

}


void mc_handler(std::string host) {

    //AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");


    //for (auto const& kv : exchKeys.declared) {
    //    setup_exchange(connection, kv.first, kv.second);
    //}

    //start.lock();
    //start.unlock();

//    int uart0_filestream =-1;
//
//    wiringPiSetupGpio ();  //This lets us use broad com pin numberings to set pin values
//    pinMode (SYNC,INPUT);
//    pinMode (SYNC2,OUTPUT);
//    pinMode (SYNC3,INPUT);
//    digitalWrite(SYNC2,LOW);
//    wiringPiISR(SYNC, INT_EDGE_RISING,&interupt1);

    while(getRunning()){
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        std::cout << "Motor control is alive" << std::endl;

//        std::cout << "Motor control running" << std::endl;

        _mutexes.commands.lock();

        Command* toSend = nullptr;

        if (!getFlag()) {
            if (_data.commands != nullptr) {
                std::cout << "Pulling command" << std::endl;
                toSend = _data.commands->remove();
            }

            if (toSend == nullptr) {
                std::cout << "Using default command" << std::endl;
                toSend = _data.command;
            }
        } else {
            toSend = new Command(getLinear(), getAngular(), 0, 0, 0, 1);
        }

        std::cout << toSend->angvel << std::endl;
        std::cout << toSend->linvel << std::endl;

        _mutexes.commands.unlock();


//        Status *status = new Status(exchKeys.GPS, getRunning(), "normal");
//        std::string status_mess = Processor::encode_status(*status);
//        send_message(connection, status_mess, statusInfo.header, statusInfo.exchange, statusInfo.key);
    }

}


//rc control input
void rcInput() {
	//sets up wiringPi and pins
	 wiringPiSetupGpio();// Duplicate
	setPWMpin(estop);
	setPWMpin(channelA);
	setPWMpin(channelB);
	
	//rc control code runs in this function
	//currently never will stop, needs something here.
	//calibration in for loop below.
	for(int i =  0; i < 10; i++){
		centa = readPWM(channelA);
		centb = readPWM(channelB);
	}
    int i = 0;
	while(i < 200) {
		rcControl();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        i++;
	}
    setFlag(false);
    std::cout << "SET FALSE SET FALSE SET FALSE" << std::endl;
}

TCLAP::CmdLine cmd("Motor control client code", ' ', "0.0");
TCLAP::ValueArg<std::string> ipArg("i","ip", "IP RabbitMQ-Server lives on", false, "amqp://localhost", "string");
TCLAP::SwitchArg motorsArg("m", "motors", "myrio connected", cmd, false);
TCLAP::SwitchArg motionArg("n", "motion", "motion planner reporting", cmd, false);

int main(int argc, char** argv) {

    cmd.add(ipArg);

    cmd.parse(argc, argv);

    _data.command = new Command(0.5, 0, 0, 0, 0, 1);
    std::string host = ipArg.getValue();

    exchange_keys.insert({exchKeys.gps_exchange, exchKeys.gps_key});
    exchange_keys.insert({exchKeys.control_exchange, exchKeys.mc_key});
    exchange_keys.insert({exchKeys.nav_exchange, exchKeys.mc_key});

//    std::thread sub(mc_subscriber, host);
    std::thread pub(mc_handler, host);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	std::thread remote(rcInput); //is this correct for making thread, YES


 //   sub.join();
    pub.join();
	remote.join();

}
