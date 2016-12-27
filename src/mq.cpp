//
// Created by armin1215 on 9/22/16.
//

#include "../include/mq.h"

using namespace rapidjson;

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
                       std::string exchange, std::string key, bool json) {
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(messageHeaders.WCLOSE)}};
    b_message->HeaderTable(header_table);

    conn->BasicPublish(exchange, key, b_message, false, false);
}

void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key, bool json) {

    // Option 2 without headers I want
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(messageHeaders.WGPSFRAME)}};
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

void GPSMessage::Serialize(Writer<StringBuffer> &writer) const {
    writer.StartObject();
    writer.Key("lat");
    writer.Double(lat);

    writer.Key("lon");
    writer.Double(lon);

    writer.Key("time");
    writer.String(time.c_str());
}

GPSMessage::GPSMessage(Document &d, bool preprocessed) {
    if (preprocessed) {
        Value& _lon = d["lon"];
        Value& _lat = d["lat"];
        Value& _time = d["time"];


        lat = _lat.GetDouble();
        lon = _lon.GetDouble();
        time = _time.GetString();
    } else {
        lat = GetValueByPointer(d, "/data/lat")->GetDouble();

        std::cout << lat << std::endl;
        lon = GetValueByPointer(d, "/data/lon")->GetDouble();

        std::cout << lon << std::endl;
        time = d["time"].GetString();

        std::cout << time << std::endl;
    }

}

GPSMessage::~GPSMessage() = default;


std::string Processor::encode_gps(GPSMessage& to_encode) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    to_encode.Serialize(writer);
    return buffer.GetString();
}

GPSMessage* Processor::decode_gps(std::string to_decode, bool preprocessed) {
    Document d;
    d.Parse(to_decode.c_str());
    return new GPSMessage(d, preprocessed);
}