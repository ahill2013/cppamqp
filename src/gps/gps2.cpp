#include "../../include/mq.h"
#include "../../include/processor.h"
#include <stdio.h>

// DO NOT USE WORKS NOT HOW IT IS DONE NOW

MessageHeaders headers;
ExchKeys exchKeys;

// Mutexes to protect global data
struct Mutexes {
    std::mutex m_num;
} _mutexes;


// global data fields that need to be protected
// Place gps information in here, control commands in here
struct Data {
    int message_num = 0;
} data;

std::map<std::string, std::string> exchange_keys;


// These are memory protecting data accesses
// Struct should not be directly accessed
void setMessage() {
    _mutexes.m_num.lock();
    data.message_num += 1;
    _mutexes.m_num.unlock();
}

// Memory protected access
int getMessage() {
    _mutexes.m_num.lock();
    int num = data.message_num;
    _mutexes.m_num.unlock();
    return num;
}

int sent = 0;
int delivered = 0;

struct ev_loop* sub_loop = ev_loop_new();


/**
 * This is where all GPS computations will take place. Check data fields for changes on loop. If state of the
 * robot has changed respond accordingly.
 *
 * Set up a loop to publish the location, acceleration, velocity, etc. once every tenth of a second.
 * TODO: Periodically send a status update to the Control unit
 * @param host string name of the host with the ip:port number as necessary
 */
void gps_publisher(std::string host) {

    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");

    for (auto const& kv : exchKeys.declared) {
        setup_exchange(connection, kv.first, kv.second);
    }

    int _iterations = 0;

    // Turn this into a while(true) loop to keep posting messages
    while (_iterations < 10) {

        Document jsonDoc;
        std::string message ="{\"data\": {\"sender\": 13881, \"msg_type\": 256, \"wn\": 1787, "
                "\"tow\": 478500, \"crc\": 54878, \"length\": 11, \"flags\": 0, \"ns\": 0, \"preamble\": 85, "
                "\"payload\": \"+wYkTQcAAAAAAAA=\", \"lon\": -122.17203108848562, \"lat\": 37.430193934253346},"
                " \"time\": \"2016-10-13T21:49:54.208732\"}";
        jsonDoc.Parse(message.c_str());


        GPSMessage* gpsMessage = new GPSMessage(jsonDoc, false);
        send_message(connection, Processor::encode_gps(*gpsMessage), headers.WGPSFRAME, exchange, key);
        std::cout << _iterations << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        setMessage();
        _iterations++;
    }

    std::string closing = "closing";
    close_message(connection, closing, exchange, key);
}

// Listen for incoming information like commands from Control or requests from other components
void gps_subscriber(std::string host) {
    MessageHeaders headers1;

    std::string queue = exchKeys.gps_sub;

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

        delivered++;

        // Do not remove this ever
        chan->ack(deliveryTag);
        std::string header = message.headers().get("MESSAGE");

        std::cout << header.c_str() << std::endl;
        std::cout << getMessage() << std::endl;
        setMessage();

        if (header == headers1.WCLOSE) {
            std::cout << "Supposed to close" << std::endl;
        }
    };

    // Must do this for each set of exchanges and queues that are being listened to


    for (auto const & kv : exchange_keys) {
        setup_consumer(chan, subscriber->getQueue(), kv.first, kv.second);
    }
    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);    // Start consuming messages

    ev_run(sub_loop, 0);    // Run event loop ev_unloop(sub_loop) will kill the event loop

}

int main() {
    std::string host = "amqp://localhost/";

    exchange_keys.insert({exchKeys.gps_exchange,exchKeys.gps_key});
    std::thread sub(gps_subscriber, host);
    std::thread pub(gps_publisher, host);

    sub.join();
    pub.join();

}
