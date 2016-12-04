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
#include "thread"
#include "mutex"
#include "chrono"

// Send a message to an exchange with a key
void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key, bool json);

// Tell subscribers to either unbind or to shut themselves down
void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                  std::string exchange, std::string key, bool json);

struct MessageHeaders {
    std::string GENERICNAME = "MESSAGE";
    std::string CLOSE = "CLOSE";
    std::string GPSFRAME = "GPS";
    std::string STATUS = "STATUS";
    std::string GETSTATUS = "GETSTATUS";
};

// Exchange key pair to bind to
struct Pair {
    std::string exchange;
    std::string key;
};


struct ExchKeys {
    std::string FANOUT = "fanout";
    std::string TOPIC = "topic";
    std::string DIRECT = "direct";

    std::string gps_exchange = "GPS";
    std::string gps_key = "gps_key";

    std::string mc_exchange = "MC";
    std::string mc_key = "mc_key";


    std::string gps_sub = "gps_sub_queue";
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

#endif //AMQPTRIAL_MQ_H