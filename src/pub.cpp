#include "../include/mq.h"


void publish(struct ev_loop* loop, AMQP::TcpChannel* chan, std::string exch, std::string key) {

    int i = 0;
    std::cout << exch << std::endl;
    std::cout << key << std::endl;
    while (i < 2000) {
        chan->startTransaction();

        chan->publish(exch, key, "Publishing Message Now");

        chan->commitTransaction();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << i << std::endl;
        i++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    ev_unloop(loop, 0);
}

int main()
{
    ExchKeys exchKeys;
    std::cout << exchKeys.gps_exchange << std::endl;
    std::string host = "amqp://localhost/";
    struct ev_loop *loop = EV_DEFAULT;
    std::string q = "subscribe";
    std::string e = "myexchange";
    std::string r = "mykey";
    MQSub* subscriber = new MQSub(*loop, host, q, e, r);

    std::string qp = "publish";
    std::string ep = "myexchange";
    std::string rp = "mykey";
    MQPub* publisher = new MQPub(*loop, host, qp, ep, rp);

//    std::cout << "Here" << std::endl;
    // create a temporary queue
    AMQP::TcpConnection* conn = subscriber->getConnection();
    AMQP::TcpChannel* chan = subscriber->getChannel();



    chan->declareExchange(e, AMQP::topic);
    chan->declareQueue(qp);
    chan->declareQueue(q);
    chan->bindQueue("myexchange", q, r);

    std::cout << subscriber->getQueue() << std::endl;
    std::cout << subscriber->getExchange() << std::endl;
    std::cout << subscriber->getKey() << std::endl;

    std::cout << "Here" << std::endl;

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
    auto messageCb = [chan](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {

        std::cout << "message received" << std::endl;

        // acknowledge the message
        chan->ack(deliveryTag);
        std::cout << message.body() << std::endl;
//        std::this_thread::sleep_for(500);
    };

    std::cout << chan->connected() << std::endl;
//    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);

    std::thread t(publish, loop, chan, ep, rp);
//    bool i = chan->publish(publisher->getExchange(), publisher->getKey(), "test");


    // run the loop
    ev_run(loop,0);


    std::cout << "Fishies" << std::endl;

    return 0;
}
