#include "../include/mq.h"
#include <stdio.h>

MessageHeaders headers;
// Mutexes to protect global data
struct Mutexes {
    std::mutex m_num;
} _mutexes;


// global data fields that need to be protected
// Place gps information in here, control commands in here
struct Data {
    int message_num = 0;
} data;


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
    ExchKeys exchKeys;

    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");
    connection->DeclareExchange(exchange, exchKeys.FANOUT, false, false, false);

    int _iterations = 0;

    std::string message = "my_message";

    while (_iterations < 10) {

        send_message(connection, message, headers.WGPSFRAME, exchange, key, false);
        std::cout << _iterations << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        setMessage();
        _iterations++;
    }

    close_message(connection, message, exchange, key, false);
}

// Listen for incoming information like commands from Control or requests from other components
void gps_subscriber(std::string host) {
//    MessageHeaders headers;
    ExchKeys exchKeys;

    std::string queue = exchKeys.gps_sub;
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

    MQSub* subscriber = new MQSub(*sub_loop, host, queue, exchange, key);
    AMQP::TcpChannel* chan = subscriber->getChannel();

    auto startCb = [](const std::string& consumertag) {
        std::cout << "Consumer operation started" << std::endl;
    };

    auto errorCb = [](const char *message) {
        std::cout << message << std::endl;
        std::cout << "Consume operation failed" << std::endl;
    };

    // Handle commands. Every time a message arrives this is called
    auto messageCb = [chan, headers](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "Message Delivered" << std::endl;

        delivered++;

        // Do not remove this ever
        chan->ack(deliveryTag);

//        std::cout << message.headers() << std::endl;
//        std::cout << message.headers().get("MESSAGE") << std::endl;
        std::string header = message.headers().get("MESSAGE");

        std::cout << header.c_str() << std::endl;
//        std::cout << delivered << std::endl;
        std::cout << getMessage() << std::endl;
        setMessage();

        if (header == headers.WCLOSE) {
            std::cout << "Supposed to close" << std::endl;
        }
    };

    // Must do this for each set of exchanges and queues that are being listened to
    chan->declareExchange(exchange, AMQP::fanout);  // Make sure exchange exists
    chan->declareQueue(queue);  // Declare a queue
    chan->bindQueue(exchange, queue, key);  // Bind a queue to an exchange so that it gets messages posted on exchange



    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);    // Start consuming messages

    ev_run(sub_loop, 0);    // Run event loop ev_unloop(sub_loop) will kill the event loop

//    ev_loop_destroy(sub_loop);
}

int main() {
    std::string host = "amqp://localhost/";
    std::thread sub(gps_subscriber, host);
    std::thread pub(gps_publisher, host);

    sub.join();
    pub.join();

}
