#include "../../include/mq.h"
#include "../../include/processor.h"
#include <stdio.h>

MessageHeaders headers;
ExchKeys exchKeys;

// Mutexes to protect global data
struct Mutexes {
    std::mutex m_num;
} _mutexes;


// global data fields that need to be protected
// Place gps information in here, control commands in here
struct Data {
    int message_num = 0;
} data;

std::map<std::string, std::string> exchange_keys;


// These are memory protecting data accesses
// Struct should not be directly accessed
void setMessage() {
    _mutexes.m_num.lock();
    data.message_num += 1;
    _mutexes.m_num.unlock();
}

// Memory protected access
int getMessage() {
    _mutexes.m_num.lock();
    int num = data.message_num;
    _mutexes.m_num.unlock();
    return num;
}

int sent = 0;
int delivered = 0;

struct ev_loop* sub_loop = ev_loop_new();


/**
 * This is where all GPS computations will take place. Check data fields for changes on loop. If state of the
 * robot has changed respond accordingly.
 *
 * Set up a loop to publish the location, acceleration, velocity, etc. once every tenth of a second.
 * TODO: Periodically send a status update to the Control unit
 * @param host string name of the host with the ip:port number as necessary
 */
void nav_publisher(std::string host) {

    AmqpClient::Channel::ptr_t connection = AmqpClient::Channel::Create("localhost");

    for (auto const& kv : exchKeys.declared) {
        setup_exchange(connection, kv.first, kv.second);
    }

    int _iterations = 0;

    // Turn this into a while(true) loop to keep posting messages
    while (_iterations < 10) {

        Document jsonDoc;
        std::string message ="{\"data\": {\"sender\": 13881, \"msg_type\": 256, \"wn\": 1787, "
                "\"tow\": 478500, \"crc\": 54878, \"length\": 11, \"flags\": 0, \"ns\": 0, \"preamble\": 85, "
                "\"payload\": \"+wYkTQcAAAAAAAA=\", \"lon\": -122.17203108848562, \"lat\": 37.430193934253346},"
                " \"time\": \"2016-10-13T21:49:54.208732\"}";
        jsonDoc.Parse(message.c_str());


        GPSMessage* gpsMessage = new GPSMessage(jsonDoc, false);
        send_message(connection, Processor::encode_gps(*gpsMessage), headers.WGPSFRAME, exchKeys.gps_exchange, exchKeys.gps_key);
        std::cout << _iterations << std::endl;

        long time = 100010001000001;
        Lines* lines = new Lines(10, 10, time);

        Line ex;
        ex.beginX = -1.0;
        ex.beginY = -1.0;
        ex.endX = 1.0;
        ex.endY = 1.0;

        Line ex2;
        ex2.beginX = -100000.0;
        ex2.beginY = -100000.0;
        ex2.endX = 100000.0;
        ex2.endY = 100000.0;

        lines->addLine(ex);
        lines->addLine(ex2);

        std::string linesMessage = Processor::encode_lines(*lines);

        send_message(connection, linesMessage, headers.WLINES, exchKeys.vision_exchange, exchKeys.nav_key);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        setMessage();
        _iterations++;
    }

//    std::string closing = "closing";
//    close_message(connection, closing, ex, key);
}

int main() {
    std::string host = "amqp://localhost/";

//    exchange_keys.insert({exchKeys.gps_exchange,exchKeys.gps_key});
    nav_publisher(host);

}