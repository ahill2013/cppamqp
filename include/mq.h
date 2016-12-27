//
// Created by armin1215 on 9/22/16.
//

#ifndef AMQPTRIAL_MQ_H
#define AMQPTRIAL_MQ_H

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include "SimpleAmqpClient/SimpleAmqpClient.h"

#include "../lib/rapidjson/document.h"
#include "../lib/rapidjson/writer.h"
#include "../lib/rapidjson/stringbuffer.h"
#include "../lib/rapidjson/pointer.h"
#include "thread"
#include "mutex"
#include "chrono"

void setup_consumer(AMQP::TcpChannel* chan, std::string queue, std::string exchange, std::string key);
void setup_exchange(AmqpClient::Channel::ptr_t conn, std::string exchange, std::string type);
void setup_cop_exchange(AMQP::TcpChannel* chan, std::string exchange, std::string type);
// Send a message to an exchange with a key
void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key, bool json);

// Tell subscribers to either unbind or to shut themselves down
void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                  std::string exchange, std::string key, bool json);

struct MessageHeaders {

    // ASCII versions of headers
    std::string GENERICNAME = "MESSAGE";
    std::string CLOSE = "CLOSE";
    std::string GPSFRAME = "GPS";
    std::string STATUS = "STATUS";
    std::string GETSTATUS = "GETSTATUS";

    // UNICODE versions of headers
    std::string WGENERICNAME = u8"MESSAGE";
    std::string WCLOSE = u8"CLOSE";
    std::string WGPSFRAME = u8"GPS";
    std::string WSTATUS = u8"STATUS";
    std::string WGETSTATUS = u8"GETSTATUS";


};

struct ExchKeys {
    std::string FANOUT = "fanout";
    std::string TOPIC = "topic";
    std::string DIRECT = "direct";

    std::string gps_exchange = "GPS";
    std::string gps_key = "gps_key";

    std::string mc_exchange = "MC";
    std::string mc_key = "mc_key";

    std::string vision_exchange = "VISION";
    std::string vision_key = "vision_key";

    std::string control_exchange = "CONTROL";
    std::string control_key = "control_key";

    std::string nav_exchange = "NAV";
    std::string nav_key = "nav_key";


    std::string gps_sub = "gps_sub_queue";
    std::string mc_sub = "mc_sub_queue";
    std::string vision_sub = "vision_sub_queue";

    std::map<std::string,std::string> declared = {{gps_exchange, FANOUT}, {mc_exchange, TOPIC}};

};


class MQSub {
public:
    MQSub(struct ev_loop&, std::string& host, std::string& queue, std::string &exchange, std::string &routingKey);
    ~MQSub();

    AMQP::TcpChannel* getChannel();
    AMQP::TcpConnection* getConnection();

    std::string getQueue();
    std::string getExchange();
    std::string getKey();

    void consume();

private:
    AMQP::LibEvHandler* handler;
    AMQP::TcpConnection* conn;
    AMQP::TcpChannel* chan;

    std::string _host;
    std::string _queue;
    std::string _exchange;
    std::string _routingKey;

};


using namespace rapidjson;

class GPSMessage {
public:
    float lat;
    float lon;
    double linvel;
    double linacc;
    double angvel;
    double angacc;
    std::string time;

    void Serialize(Writer<StringBuffer>& writer) const;

    GPSMessage(Document& d, bool);
    ~GPSMessage();
};
// Put all message types (as structs) in here, include ways to send and parse them)
class Processor {
public:
    // Repeat one of these per message
    std::string encode_gps(GPSMessage& to_encode);
    GPSMessage* decode_gps(std::string to_decode, bool);
private:
    Document reader;
};


#endif //AMQPTRIAL_MQ_H