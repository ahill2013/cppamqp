//
// Created by armin1215 on 9/22/16.
//

#ifndef AMQPTRIAL_MQ_H
#define AMQPTRIAL_MQ_H

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include "SimpleAmqpClient/SimpleAmqpClient.h"

#include "thread"
#include "mutex"
#include "chrono"
#include <assert.h>

void setup_consumer(AMQP::TcpChannel* chan, std::string queue, std::string exchange, std::string key);
void setup_exchange(AmqpClient::Channel::ptr_t conn, std::string exchange, std::string type);
void setup_cop_exchange(AMQP::TcpChannel* chan, std::string exchange, std::string type);
// Send a message to an exchange with a key
void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key);

// Tell subscribers to either unbind or to shut themselves down
void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                  std::string exchange, std::string key);

struct MessageHeaders {

    // ASCII versions of headers
//    std::string GENERICNAME = "MESSAGE";
//    std::string CLOSE = "CLOSE";
//    std::string GPSFRAME = "GPS";
//    std::string NAV = "NAV";
//    std::string VISUAL = "VISUAL";
//    std::string STATUS = "STATUS";
//    std::string GETSTATUS = "GETSTATUS";

    // UNICODE versions of headers
    std::string WGENERICNAME = u8"MESSAGE";
    std::string WCLOSE = u8"CLOSE";
    std::string WSTART = u8"START";
    std::string WNAV = u8"NAV";

    std::string WSTATUS = u8"STATUS";
    std::string WGETSTATUS = u8"GETSTATUS";

    std::string WGPSFRAME = u8"GPS";
    std::string WCOMMANDS = u8"COMMANDS";
    std::string WINTERVAL = u8"INTERVAL";
    std::string WLINES = u8"LINES";
    std::string WMBROAD = u8"MOTORBROAD";
    std::string WOBSTACLES = u8"OBSTACLES";


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

    std::map<std::string,std::string> declared = {{gps_exchange, FANOUT}, {mc_exchange, TOPIC},
                                                  {control_exchange, TOPIC}, {nav_exchange, TOPIC},
                                                  {vision_exchange, TOPIC}};

};


//enum header_code {
//    egps,
//    ecom,
//    eclose
//};
//
//std::map<std::string, header_code> map = {
//        { mapHeaders.WGPSFRAME, header_code::egps},
//        {mapHeaders.WCOMMANDS, header_code ::ecom},
//        {mapHeaders.WCLOSE, header_code ::eclose}
//};
//
//
//header_code hash(std::string header) {
//    return map->
//}


class MQSub {
public:
    MQSub(struct ev_loop&, std::string& host, std::string& queue);
    ~MQSub();

    AMQP::TcpChannel* getChannel();
    AMQP::TcpConnection* getConnection();

    std::string getQueue();

private:
    AMQP::LibEvHandler* handler;
    AMQP::TcpConnection* conn;
    AMQP::TcpChannel* chan;

    std::string _host;
    std::string _queue;

};

#endif //AMQPTRIAL_MQ_H