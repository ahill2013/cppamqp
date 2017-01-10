//
// Created by armin1215 on 12/4/16.
//

#include "../include/mq.h"
#include <stdio.h>

int main() {
    std::string demo_mess = "In this demo we will receive a message from some source and place it into a Document."
            "After placing the message into a document we shall decode it into a GPSMessage, which we will use. Then we"
            "will re-encode that information into an actual JSON GPSMessage and send it to be used by someone else.";

    std::cout << demo_mess << std::endl;


    std::string message ="{\"data\": {\"sender\": 13881, \"msg_type\": 256, \"wn\": 1787, "
            "\"tow\": 478500, \"crc\": 54878, \"length\": 11, \"flags\": 0, \"ns\": 0, \"preamble\": 85, "
            "\"payload\": \"+wYkTQcAAAAAAAA=\", \"lon\": -122.17203108848562, \"lat\": 37.430193934253346},"
            " \"time\": \"2016-10-13T21:49:54.208732\"}";


    std::cout << "\nORIGINAL RTK MESSAGE" << std::endl;
    std::cout << message << std::endl;

    // Place the document into a string
    Document d;
    d.Parse(message.c_str());

//    std::cout << message.c_str() << std::endl;
//    std::cout << "Here" << std::endl;


    // This is how you write a json message to a JSON document
    // 1) Create a string buffer or empty an already existing string buffer
    // 2) Assign the buffer to a Writer
    // 3) Add the writer to the RapidJSON Document
    StringBuffer buffer;
    buffer.Clear(); // Empty the buffer
    Writer<StringBuffer> writer(buffer); // Assign the buffer to a RapidJSON Writer
    d.Accept(writer); // Add the Writer to the document

//    std::cout << buffer.GetString() << std::endl;
    Processor* processor = new Processor();

    std::cout.precision(21); // Set decimal precision. Numbers are right but it doesn't show up when you print always

    std::cout << "\nRTK MESSAGE PARSED INTO GPS MESSAGE" << std::endl;
    GPSMessage* gpsMessage = processor->decode_gps(message, false);
    printf("Lat: %.10f\n", gpsMessage->lat);
    printf("Lon: %.10f\n", gpsMessage->lon);
    printf("Time: %s\n", gpsMessage->time.c_str()); // Insecure, I know


    std::cout << "\nENCODED INTO GPSMESSAGE TO BE USED BY MOTORS, NAV, ETC." << std::endl;
    std::string encoded = processor->encode_gps(*gpsMessage);
    std::cout << encoded << std::endl;

    GPSMessage* gpsMessage1 = processor->decode_gps(encoded, true);

    std::cout << "\nDECODED INTO GPSMESSAGE CLASS" << std::endl;
//    std::cout << encoded << std::endl;
    std::cout << "Lat: " << gpsMessage1->lat << std::endl;
    std::cout << "Lon: " << gpsMessage1->lon << std::endl;
    std::cout << "Time: " << gpsMessage1->time << std::endl;



    long time = 1000000001;
    Visual* visual = new Visual(time);

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

    Obstacle ob;
    ob.radius = 200;
    ob.x = -10.0;
    ob.y = -10.0;
    ob.type = 1;

    Obstacle ob2;
    ob2.radius = -200;
    ob2.x = 10.0;
    ob2.y = 10.0;
    ob2.type = 3;

    visual->addLine(ex);
    visual->addLine(ex2);
    visual->addObstacle(ob);
    visual->addObstacle(ob2);

    std::cout << "\n" << processor->encode_vision(*visual) << std::endl;

}