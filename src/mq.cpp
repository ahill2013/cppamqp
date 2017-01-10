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

// Simple writer
void GPSMessage::Serialize(Writer<StringBuffer> &writer) const {
    writer.StartObject();
    writer.String("lat");
    writer.Double(lat);

    writer.String("lon");
    writer.Double(lon);

    writer.String("time");
    writer.String(time.c_str(), static_cast<SizeType>(time.length()));

    writer.EndObject();
}

GPSMessage::GPSMessage(Document &d, bool preprocessed) {
    if (preprocessed) {
        std::cout << "Preprocessed" << std::endl;

        lat = GetValueByPointer(d, "/lat")->GetDouble();
        lon = GetValueByPointer(d, "/lon")->GetDouble();
        time = GetValueByPointer(d, "/time")->GetString();
    } else {
        lat = GetValueByPointer(d, "/data/lat")->GetDouble();

//        std::cout << lat << std::endl;
        lon = GetValueByPointer(d, "/data/lon")->GetDouble();

//        std::cout << lon << std::endl;
        time = d["time"].GetString();

//        std::cout << time << std::endl;
    }

}

GPSMessage::~GPSMessage() = default;


////////////////////////////////////////////////////////////////////////


Visual::Visual(long time) {
    this->time = time;
    lines = new std::vector<Line>();
    obstacles = new std::vector<Obstacle>();
}

void Visual::addLine(Line& line) {
    lines->push_back(line);
}

void Visual::addObstacle(Obstacle& obstacle) {
    obstacles->push_back(obstacle);
}


void Visual::Serialize(Writer<StringBuffer> &writer) const {
    Document jsonDoc;
    jsonDoc.SetObject();
    Document::AllocatorType& allocator = jsonDoc.GetAllocator();

    Value linesArray(kArrayType);
    Value obsArray(kArrayType);


    for (Line& line : *lines) {
        Value lineVal;
        lineVal.SetObject();
        lineVal.AddMember("beginX", line.beginX, allocator);
        lineVal.AddMember("beginY", line.beginY, allocator);
        lineVal.AddMember("endX", line.endX, allocator);
        lineVal.AddMember("endY", line.endY, allocator);
        linesArray.PushBack(lineVal, allocator);
    }

    for (Obstacle& obstacle : *obstacles) {
        Value obsVal;
        obsVal.SetObject();
        obsVal.AddMember("x", obstacle.x, allocator);
        obsVal.AddMember("y", obstacle.y, allocator);
        obsVal.AddMember("type", obstacle.type, allocator);
        obsVal.AddMember("radius", obstacle.radius, allocator);
        obsArray.PushBack(obsVal, allocator);
    }

    jsonDoc.AddMember("lines", linesArray, allocator);
    jsonDoc.AddMember("obstacles", obsArray, allocator);

    jsonDoc.Accept(writer);
}

Visual::~Visual() {
    delete lines;
    delete obstacles;
}

///////////////////////////////////////////////////////////////////////////

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

std::string Processor::encode_vision(Visual &to_encode) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    to_encode.Serialize(writer);
    return buffer.GetString();
}