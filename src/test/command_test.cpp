#include "../../include/mq.h"
#include "../../include/processor.h"
#include <stdio.h>

// DEPRECATED DO NOT USE

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

// Listen for incoming information like commands from Control or requests from other components
void gps_subscriber(std::string host) {
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

        delivered++;

        // Do not remove this ever
        chan->ack(deliveryTag);
        std::string header = message.headers().get("MESSAGE");

        std::cout << header.c_str() << std::endl;

        std::string mess = message.message();
        std::cout << mess.c_str() << std::endl;
        if (header == headers.WCOMMANDS) {
            Commands* commands = Processor::decode_commands(mess);

            for (auto iter = commands->commands->begin(); iter != commands->commands->end(); ++iter) {
                Command* command = *iter;

                std::cout << command->angvel << std::endl;
                std::cout << command->duration << std::endl;
            }
        }

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

    exchange_keys.insert({exchKeys.nav_exchange,exchKeys.mc_key});
    exchange_keys.insert({exchKeys.control_exchange, exchKeys.mc_key});
    std::thread sub(gps_subscriber, host);
//    std::thread pub(gps_publisher, host);

    sub.join();
//    pub.join();

}



