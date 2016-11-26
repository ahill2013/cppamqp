//
// Created by armin1215 on 9/22/16.
//

#ifndef AMQPTRIAL_MQ_H
#define AMQPTRIAL_MQ_H

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include "../lib/rapidjson/document.h"
#include "../lib/rapidjson/writer.h"
#include "../lib/rapidjson/stringbuffer.h"
#include "thread"
#include "mutex"
#include "chrono"


struct ExchKeys {
    std::string gps_exchange = "GPS";
    std::string gps_key = "gps_key";

    std::string mc_exchange = "MC";
    std::string mc_key = "mc_key";
};


class MQPub {
public:
    MQPub(struct ev_loop&, std::string& host, std::string& queue, std::string &exchange, std::string &routingKey);
    ~MQPub();

    AMQP::TcpChannel* getChannel();
    AMQP::TcpConnection* getConnection();

    std::string getQueue();
    std::string getExchange();
    std::string getKey();

    bool publish();

private:
    AMQP::LibEvHandler* handler;
    AMQP::TcpConnection* conn;
    AMQP::TcpChannel* chan;

    std::string _host;
    std::string _queue;
    std::string _exchange;
    std::string _routingKey;

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
