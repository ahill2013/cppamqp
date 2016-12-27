//
// Created by armin1215 on 12/4/16.
//

#include "../include/mq.h"
#include <stdio.h>

int main() {
    std::string message ="{\"data\": {\"sender\": 13881, \"msg_type\": 256, \"wn\": 1787, "
            "\"tow\": 478500, \"crc\": 54878, \"length\": 11, \"flags\": 0, \"ns\": 0, \"preamble\": 85, "
            "\"payload\": \"+wYkTQcAAAAAAAA=\", \"lon\": -122.17203108848562, \"lat\": 37.430193934253346},"
            " \"time\": \"2016-10-13T21:49:54.208732\"}";

    std::cout << message << std::endl;
    Document d;
    d.Parse(message.c_str());

    std::cout << message.c_str() << std::endl;
    std::cout << "Here" << std::endl;


    StringBuffer buffer;
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    std::cout << buffer.GetString() << std::endl;
    Processor* processor = new Processor();

    std::cout << "Here" << std::endl;
    std::cout.precision(21);

    GPSMessage* gpsMessage = processor->decode_gps(message, false);
    printf("%.10f\n", gpsMessage->lat);
    std::cout << gpsMessage->lon << std::endl;
    std::cout << gpsMessage->time << std::endl;



    std::string encoded = processor->encode_gps(*gpsMessage);

    std::cout << "Here" << std::endl;

    GPSMessage* gpsMessage1 = processor->decode_gps(encoded, true);

    std::cout << encoded << std::endl;

    std::cout << gpsMessage1->lat << std::endl;
    std::cout << gpsMessage1->lon << std::endl;
    std::cout << gpsMessage1->time << std::endl;
}