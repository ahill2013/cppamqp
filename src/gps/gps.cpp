#include "../../include/mq.h"
#include "../../include/processor.h"
#include <stdio.h>

MessageHeaders headers;
ExchKeys exchKeys;

// Real GPS code

// Report status every thirty publishes
int status = 10;
std::map<std::string, std::string> exchange_keys;

struct GPSPublishInfo {
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;
    std::string header = headers.WGPSFRAME;
} gpsInfo;

struct StatusPublish {
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.control_key;
    std::string header = headers.WSTATUS;
} statusInfo;

// Mutexes to protect global data


struct Data {
    bool running = true;
    bool started = false;
    double interval = 0.1;
} _data;

struct Mutexes {
    std::mutex running;
    std::mutex started;
    std::mutex interval;
} _mutexes;


void setRunning(bool value) {
    _mutexes.running.lock();
    _data.running = value;
    _mutexes.running.unlock();
}


bool getRunning() {
    _mutexes.running.lock();
    bool running = _data.running;
    _mutexes.running.unlock();
    return running;
}

void setStarted(bool value) {
    _mutexes.started.lock();
    _data.started = value;
    _mutexes.started.unlock();
}


bool getStarted() {
    _mutexes.started.lock();
    bool started = _data.started;
    _mutexes.started.unlock();
    return started;
}


void setInterval(double interval) {
    _mutexes.interval.lock();
    _data.interval = interval;
    _mutexes.interval.unlock();
}

double getInterval() {
    _mutexes.interval.lock();
    double interval = _data.interval;
    _mutexes.interval.unlock();
    return interval;
}

struct ev_loop* sub_loop = ev_loop_new();
std::mutex start;


/**
 * This is where all GPS computations will take place. Check data fields for changes on loop. If state of the
 * robot has changed respond accordingly.
 *
 * Set up a loop to publish the location, acceleration, velocity, etc. once every tenth of a second.
 * TODO: Periodically send a status update to the Control unit
 * @param host string name of the host with the ip:port number as necessary
 */
void gps_publisher(std::string host) {

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");

    for (auto const& kv : exchKeys.declared) {
        setup_exchange(connection, kv.first, kv.second);
    }

    int _iterations = 0;

    start.lock();
    start.unlock();


    // Turn this into a while(true) loop to keep posting messages
    while (getRunning() && _iterations < 20) {
        _iterations++;
        auto start = std::chrono::high_resolution_clock::now();

        // Get gps message here and convert JSON -> GPSMessage -> std::string
        std::string message = "{\"data\": {\"sender\": 13881, \"msg_type\": 256, \"wn\": 1787, "
                "\"tow\": 478500, \"crc\": 54878, \"length\": 11, \"flags\": 0, \"ns\": 0, \"preamble\": 85, "
                "\"payload\": \"+wYkTQcAAAAAAAA=\", \"lon\": -122.17203108848562, \"lat\": 37.430193934253346},"
                " \"time\": \"2016-10-13T21:49:54.208732\"}";

        Document d;
        d.Parse(message.c_str());
        GPSMessage *gpsMessage = new GPSMessage(d, false);
        send_message(connection, Processor::encode_gps(*gpsMessage), gpsInfo.header, gpsInfo.exchange, gpsInfo.key);
        std::cout << _iterations << std::endl;

        if (_iterations % status == 0) {
            Status *status = new Status(exchKeys.GPS, getRunning(), "normal");
            std::string status_mess = Processor::encode_status(*status);
            send_message(connection, status_mess, statusInfo.header, statusInfo.exchange, statusInfo.key);
//            _iterations = 0;
        }

        std::chrono::duration<double> interval(getInterval());

        auto end = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(interval - (end - start));
    }

    std::string closing = "closing";
    close_message(connection, closing, statusInfo.exchange, statusInfo.key);
}

// Listen for incoming information like commands from Control or requests from other components
void gps_subscriber(std::string host) {
    start.lock();
    MessageHeaders headers1;

    std::string queue = exchKeys.gps_sub;

    MQSub* subscriber = new MQSub(*sub_loop, host, queue);
    AMQP::TcpChannel* chan = subscriber->getChannel();

    for (auto const& kv : exchKeys.declared) {
        setup_cop_exchange(chan, kv.first, kv.second);
    }


    auto startCb = [](const std::string& consumertag) {
        std::cout << "Consumer operation started" << std::endl;
    };

    auto errorCb = [](const char *message) {
        std::cout << message << std::endl;
        std::cout << "Consume operation failed" << std::endl;
    };

    // Handle commands. Every time a message arrives this is called
    auto messageCb = [chan, headers1](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "Message Delivered" << std::endl;

        // Do not remove this ever
        chan->ack(deliveryTag);
        std::string header = message.headers().get("MESSAGE");

        std::cout << header.c_str() << std::endl;
        std::cout << message.message() << std::endl;
        if (header == headers1.WGPSFRAME) {
            GPSMessage* gpsMessage = Processor::decode_gps(message.message(), true);
            std::cout << gpsMessage->lat << std::endl;
//            delete gpsMessage;
        } else if (header == headers1.WINTERVAL) {
            Interval* interval = Processor::decode_interval(message.message());
            setInterval(interval->interval);
//            delete interval;
        } else if (header == headers1.WSTATUS) {
            Status* status = Processor::decode_status(message.message());
            std::cout << status->message << std::endl;
        } else if (header == headers1.WCLOSE) {
            setRunning(false);
            std::cout << "Supposed to close" << std::endl;
        } else if (header == headers1.WSTART) {
            start.unlock();
        }
    };

    // Must do this for each set of exchanges and queues that are being listened to


    for (auto const & kv : exchange_keys) {
        setup_consumer(chan, subscriber->getQueue(), kv.first, kv.second);
    }
    chan->consume(subscriber->getQueue()).onReceived(messageCb).onSuccess(startCb).onError(errorCb);    // Start consuming messages

    ev_run(sub_loop, 0);    // Run event loop ev_unloop(sub_loop) will kill the event loop

}

int main() {
    std::string host = "amqp://localhost/";

    exchange_keys.insert({exchKeys.gps_exchange, exchKeys.gps_key});
    exchange_keys.insert({exchKeys.control_exchange, exchKeys.gps_key});

    std::thread sub(gps_subscriber, host);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::thread pub(gps_publisher, host);

    sub.join();
    pub.join();

}
