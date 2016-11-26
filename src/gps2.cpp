
#include "../include/mq.h"


// global data fields that need to be protected
struct Data {
    int message_num = 0;
} data;


// These are memory protecting data accesses
// Struct should not be directly accessed
void setMessage(int x) {
    data.message_num = x;
}

int getMessage() {
    return data.message_num;
}

int sent = 0;
int delivered = 0;

struct ev_loop* loop1 = ev_loop_new();
struct ev_loop* loop2 = ev_loop_new();

std::mutex m;
//std::mutex lock;

// All actual computations and publishes concerning GPS should happen here
// Write a loop to publish all GPS informaiton every tenth of a second with a timestamp
void gps_computations(struct ev_loop *loop, AMQP::TcpChannel* chan, int iterations) {
    ExchKeys exchKeys;
    int _iterations = 0;

    while (_iterations < iterations) {

        // If you want to publish multiple messages at once start a transaction
        chan->startTransaction();
        m.lock();
        chan->publish(exchKeys.gps_exchange, exchKeys.gps_key, std::to_string(0));
        m.unlock();
        std::cout << "sending message" << std::endl;
        sent++;

        // Actual publishes won't happen until you commit the transaction
        chan->commitTransaction();

        // If you want time in between publishes use chrono
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        _iterations++;
    }

    std::cout << "Sent: " << sent << std::endl;

}

void gps_publisher(std::string host) {
    ExchKeys exchKeys;

//    loop1 = EV_DEFAULT;

    std::string queue = "gps_pub_queue";
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

    MQPub* publisher = new MQPub(*loop1, host, queue, exchange, key);
    AMQP::TcpChannel* chan = publisher->getChannel();
    chan->declareExchange(exchange, AMQP::topic);

    std::thread comp(gps_computations, loop1, publisher->getChannel(), 2000);

    ev_run(loop1, 0);
    comp.join();
}

void gps_subscriber(std::string host) {
    ExchKeys exchKeys;

    std::string queue = "gps_sub_queue";
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

    MQSub* subscriber = new MQSub(*loop2, host, queue, exchange, key);
    AMQP::TcpChannel* chan = subscriber->getChannel();

    auto startCb = [](const std::string&consumertag) {
        std::cout << "Consumer operation started" << std::endl;
    };

    auto errorCb = [](const char *message) {
        std::cout << message << std::endl;
        std::cout << "Consume operation failed" << std::endl;
    };

    auto messageCb = [chan](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        m.lock();
        std::cout << "Message Delivered" << std::endl;

        delivered++;
        chan->ack(deliveryTag);
        std::cout << delivered << std::endl;
        m.unlock();
    };

    chan->declareQueue(queue);
    chan->declareExchange(exchange, AMQP::topic);
    chan->bindQueue(exchange, queue, key);
//    chan->setQos(20);
    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);

    ev_run(loop2, 0);
}

int main() {
    std::string host = "amqp://localhost/";
//    std::thread pub(gps_publisher, host);
    std::thread sub(gps_subscriber, host);

    sub.join();
//    pub.join();

}
