//
// Created by armin1215 on 9/22/16.
//

#include "../include/mq.h"


MQPub::MQPub(struct ev_loop& loop, std::string& host, std::string& queue, std::string& exchange, std::string& routingKey) { // : handler(&loop) {
    handler = new AMQP::LibEvHandler(&loop);
    conn = new AMQP::TcpConnection(handler, AMQP::Address(host));
    chan = new AMQP::TcpChannel(conn);
    _host = host;
    _queue = queue;
    _exchange = exchange;
    _routingKey = routingKey;
}

MQPub::~MQPub() {
    // Delete parameter fields then delete channel and consumer
//    delete _host;
//    delete _queue;
//    delete _exchange;
//    delete _routingKey;

    chan->close();
    conn->close();
    delete chan;
    delete conn;
    delete handler;
}

AMQP::TcpChannel* MQPub::getChannel() {
    return this->chan;
}

AMQP::TcpConnection* MQPub::getConnection() {
    return this->conn;
}

std::string MQPub::getQueue() {
    return _queue;
}

std::string MQPub::getExchange() {
    return _exchange;
}

std::string MQPub::getKey() {
    return _routingKey;
}

bool MQPub::publish() {

// publish a number of messages
    chan->publish(getExchange(), getKey(), "my first message");
    return 1;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

MQSub::MQSub(struct ev_loop& loop, std::string& host, std::string &queue, std::string &exchange, std::string &routingKey) { // : handler(&loop) {
    handler = new AMQP::LibEvHandler(&loop);
    conn = new AMQP::TcpConnection(handler, AMQP::Address(host));
    chan = new AMQP::TcpChannel(conn);

    _host = host;
    _queue = queue;
    _exchange = exchange;
    _routingKey = routingKey;

}

MQSub::~MQSub() {
    chan->close();
    conn->close();
    delete chan;
    delete conn;
    delete handler;
}

AMQP::TcpChannel* MQSub::getChannel() {
    return this->chan;
}

AMQP::TcpConnection* MQSub::getConnection() {
    return this->conn;
}

std::string MQSub::getQueue() {
    return _queue;
}

std::string MQSub::getExchange() {
    return _exchange;
}

std::string MQSub::getKey() {
    return _routingKey;
}

void MQSub::consume() {
    AMQP::TcpChannel* channel = getChannel();
    // callback function that is called when the consume operation starts
    auto startCb = [](const std::string &consumertag) {

        std::cout << "consume operation started" << std::endl;
    };

// callback function that is called when the consume operation failed
    auto errorCb = [](const char *message) {

        std::cout << message << std::endl;
        std::cout << "consume operation failed" << std::endl;
    };

// callback operation when a message was received
    auto messageCb = [channel](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {

        std::cout << "message received" << std::endl;

        // acknowledge the message
        channel->ack(deliveryTag);
        std::cout << message.body() << std::endl;
    };

//    std::cout << channel->connected() << std::endl;
    channel->consume(getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);
}