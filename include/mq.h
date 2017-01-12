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
#include <assert.h>

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
    std::string NAV = "NAV";
    std::string VISUAL = "VISUAL";
    std::string STATUS = "STATUS";
    std::string GETSTATUS = "GETSTATUS";

    // UNICODE versions of headers
    std::string WGENERICNAME = u8"MESSAGE";
    std::string WCOMMANDS = u8"COMMANDS";
    std::string WCLOSE = u8"CLOSE";
    std::string WGPSFRAME = u8"GPS";
    std::string WNAV = u8"NAV";
    std::string WVISUAL = u8"VISUAL";
    std::string WSTATUS = u8"STATUS";
    std::string WGETSTATUS = u8"GETSTATUS";


};

struct ExchKeys {
    std::string FANOUT = "fanout";
    std::string TOPIC = "topic";
    std::string DIRECT = "direct";

    std::string gps_exchange = "gps_exchange";
    std::string gps_key = "gps_key";

    std::string mc_exchange = "mc_exchange";
    std::string mc_key = "mc_key";

    std::string vision_exchange = "vision_exchange";
    std::string vision_key = "vision_key";

    std::string control_exchange = "control_exchange";
    std::string control_key = "control_key";

    std::string log_exchange = "controllog_exchange";
    std::string log_key = "controllog_key";

    std::string nav_exchange = "nav_exchange";
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
    double lat;
    double lon;
    double linvel;
    double linacc;
    double angvel;
    double angacc;
    std::string time;
    bool ins;

    void Serialize(Writer<StringBuffer>& writer) const;

    GPSMessage(Document& d, bool);
    ~GPSMessage();
};

struct Obstacle {
    double x;
    double y;
    double radius;

    // REDFLAG, BLUEFLAG, FENCE, GENERIC
    int type;
};

struct Line {
    double beginX;
    double beginY;
    double endX;
    double endY;
};



class Visual {
public:
    double lat;
    double lon;
    unsigned long time;
    std::vector<Line>* lines;
    std::vector<Obstacle>* obstacles;

    void Serialize(Writer<StringBuffer>& writer) const;


    void addLine(Line&);
    void addObstacle(Obstacle&);


    Visual(double lat, double lon, unsigned long time);
    ~Visual();
};

class Command {
public:
    double startang;
    double linvel;
    double angvel;

    double endLat;
    double endLon;

    unsigned int duration;

    Command(const Value& val);
    ~Command();
};

class Commands {
public:
    double startLat;
    double startLon;

    std::vector<Command*>* commands;

    Commands(Document& d);
    ~Commands();
};

// Put all message types (as structs) in here, include ways to send and parse them)
class Processor {
public:
    // Repeat one of these per message
    std::string encode_gps(GPSMessage& to_encode);
    GPSMessage* decode_gps(std::string to_decode, bool);
    std::string encode_vision(Visual& to_encode);
    Commands* decode_commands(std::string to_decode);
};


#endif //AMQPTRIAL_MQ_H