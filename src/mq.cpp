//
// Created by armin1215 on 9/22/16.
//

#include "../include/mq.h"

MessageHeaders messageHeaders;

void setup_consumer(AMQP::TcpChannel* chan, std::string queue, std::string exchange, std::string key) {
    chan->declareQueue(queue);  // Declare a queue
    chan->bindQueue(exchange, queue, key);  // Bind a queue to an exchange so that it gets messages posted on exchange
}

void setup_exchange(AmqpClient::Channel::ptr_t conn, std::string exchange, std::string type) {
    conn->DeclareExchange(exchange, type, false, false, false);
}

void setup_cop_exchange(AMQP::TcpChannel* chan, std::string exchange, std::string type) {

    if (type == "fanout") {
        chan->declareExchange(exchange, AMQP::fanout);  // Make sure exchange exists
    } else if (type == "topic") {
        chan->declareExchange(exchange, AMQP::topic);
    }
}

void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                       std::string exchange, std::string key) {
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(messageHeaders.WCLOSE)}};
    b_message->HeaderTable(header_table);

    conn->BasicPublish(exchange, key, b_message, false, false);
}

void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key) {

    // Option 2 without headers I want
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(header)}};
    b_message->HeaderTable(header_table);

    conn->BasicPublish(exchange, key, b_message, false, false);
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