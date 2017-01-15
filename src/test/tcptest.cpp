// #define BOOST_NO_CXX11_RVALUE_REFERENCES
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include "thread"
#include "chrono"
#include <SimpleAmqpClient/SimpleAmqpClient.h>



struct ev_loop* loop1 = ev_loop_new();
struct ev_loop* loop2 = ev_loop_new();

void computations(AMQP::TcpChannel* chan) {
    chan->declareExchange("exchange", AMQP::topic);

    int _iterations = 0;
    while(_iterations < 20000) {
        chan->startTransaction();
        std::cout << _iterations << std::endl;
        chan->publish("exchange", "key", "message");
        chan->commitTransaction();

//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        _iterations++;
    }
}

void publisher() {
//    std::string host = "amqp://localhost/";
//    AMQP::LibEvHandler* handler = new AMQP::LibEvHandler(loop1);
//    AMQP::TcpConnection* conn = new AMQP::TcpConnection(handler, AMQP::Address(host));
//    AMQP::TcpChannel* chan = new AMQP::TcpChannel(conn);
//
//    std::thread comp(computations, chan);

    std::string host = "localhost";
    std::string exchange = "exchange";
    std::string type = "topic";

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create(host);
    connection->DeclareExchange(exchange, type, false, false, false);

    AmqpClient::BasicMessage::ptr_t message = AmqpClient::BasicMessage::Create("message");

    int _iterations = 0;
    while (_iterations < 200000) {
        connection->BasicPublish("exchange", "key", message, false, false);
        std::cout << _iterations << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        _iterations++;
    }


//    ev_run(loop1);
}

void subscriber() {
    std::string host = "amqp://localhost/";
    AMQP::LibEvHandler* handler = new AMQP::LibEvHandler(loop2);
    AMQP::TcpConnection* conn = new AMQP::TcpConnection(handler, AMQP::Address(host));
    AMQP::TcpChannel* chan = new AMQP::TcpChannel(conn);

    auto startCb = [](const std::string&consumertag) {
        std::cout << "Consumer operation started" << std::endl;
    };

    auto errorCb = [](const char *message) {
        std::cout << message << std::endl;
        std::cout << "Consume operation failed" << std::endl;
    };

    auto messageCb = [chan](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "Message Delivered" << std::endl;
        chan->ack(deliveryTag);
    };

    chan->declareQueue("queue");
    chan->declareExchange("exchange", AMQP::topic);
    chan->bindQueue("exchange", "queue", "key");

    chan->consume("queue")
            .onReceived(messageCb)
            .onSuccess(startCb)
            .onError(errorCb);

    ev_run(loop2);
}

int main() {
    std::thread sub(subscriber);
    std::thread pub(publisher);

    pub.join();
    sub.join();
}
