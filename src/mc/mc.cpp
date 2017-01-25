#include "../../include/mq.h"
#include "../../include/processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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
    Commands* command;
    GPSMessage* gpsMessage;
    bool updateGPS;
} _data;

struct Mutexes {
    std::mutex gps;
    std::mutex commands;
    std::mutex running;
    std::mutex update;
} _mutexes;


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

    if (_data.command != nullptr) {
        delete _data.command;
    }

    _data.command = commands;
    _mutexes.commands.unlock();
}

Commands* getCommands() {
    _mutexes.commands.lock();
    Commands* commands = _data.command;
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
    start.lock();
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
            Commands* commands = Processor::decode_commands(message.message());
            setCommands(commands);
            std::cout << "Got commands" << std::endl;
            Commands* comm_ret = getCommands();

            Command* comm = comm_ret->remove();

            std::cout << comm->angvel << std::endl;
            std::cout << comm->startang << std::endl;
            std::cout << comm->duration << std::endl;
        } else if (header == headers1.WGPSFRAME) {
            GPSMessage* gpsMessage = Processor::decode_gps(message.message(), true);
            setGPS(gpsMessage);
            // Code to send GPS?
        } else if (header == headers1.WCLOSE) {
            setRunning(false);
            std::cout << "Supposed to close" << std::endl;
        } else if (header == headers1.WSTART) {
            start.unlock();
        }
    };

    // Must do this for each set of exchanges and queues that are being listened to


    for (auto const & kv : exchange_keys) {
        setup_consumer(chan, subscriber->getQueue(), kv.first, kv.second);
    }
    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);    // Start consuming messages

    ev_run(sub_loop, 0);    // Run event loop ev_unloop(sub_loop) will kill the event loop

}

void mc_handler(std::string host) {

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");


    for (auto const& kv : exchKeys.declared) {
        setup_exchange(connection, kv.first, kv.second);
    }

    start.lock();
    start.unlock();

    while(getRunning()) {

        std::cout << "Running" << std::endl;

        Commands* commands = getCommands(); // DO NOT CHANGE
        if ((commands != nullptr) && !getCommands()->isEmpty()) {   // DO NOT CHANGE
            Command* to_send = commands->remove();  // DO NOT CHANGE
            _mutexes.commands.unlock();     // DO NOT CHANGE

            // put code to send command here
        } else {
            _mutexes.commands.unlock();     // DO NOT CHANGE
            std::this_thread::sleep_for(std::chrono::milliseconds(1));  // DO NOT CHANGE
        }

//        while(!getCommands()->isEmpty()) {
//            // Send information to MyRio
//            Commands* comm = getCommands();
//            Command* to_send = comm->remove();
//            // send command
//
//            // Get Response with select() or ioctl() or non-canonical termios reads
//
//            // If less than equal to zero error or timeout
//            int retval = select();
//
//            if (retval > 0) {
//
//            }
//        }
    }

}


int main() {
    std::string host = "amqp://localhost/";

    exchange_keys.insert({exchKeys.gps_exchange, exchKeys.gps_key});
    exchange_keys.insert({exchKeys.control_exchange, exchKeys.mc_key});
    exchange_keys.insert({exchKeys.nav_exchange, exchKeys.mc_key});

    std::thread sub(mc_subscriber, host);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::thread pub(mc_handler, host);

    sub.join();
    pub.join();

}
