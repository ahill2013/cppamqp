//
// Created by armin1215 on 12/4/16.
//

#include "../../include/mq.h"
#include "../../include/processor.h"
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

    std::cout.precision(12); // Set decimal precision. Numbers are right but it doesn't show up when you print always

    std::cout << "\nRTK MESSAGE PARSED INTO GPS MESSAGE" << std::endl;
    GPSMessage* gpsMessage = Processor::decode_gps(message, false);
    printf("Lat: %.10f\n", gpsMessage->lat);
    printf("Lon: %.10f\n", gpsMessage->lon);
    printf("Time: %s\n", gpsMessage->time.c_str()); // Insecure, I know


    std::cout << "\nENCODED INTO GPSMESSAGE TO BE USED BY MOTORS, NAV, ETC." << std::endl;
    std::string encoded = Processor::encode_gps(*gpsMessage);
    std::cout << encoded << std::endl;

    GPSMessage* gpsMessage1 = Processor::decode_gps(encoded, true);

    std::cout << "\nDECODED INTO GPSMESSAGE CLASS" << std::endl;
//    std::cout << encoded << std::endl;
    std::cout << "Lat: " << gpsMessage1->lat << std::endl;
    std::cout << "Lon: " << gpsMessage1->lon << std::endl;
    std::cout << "Time: " << gpsMessage1->time << std::endl;



    long time = 100010001000001;
    Lines* lines = new Lines(10, 10, time);
    Obstacles* obstacles = new Obstacles(-10, -10, time);

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

    lines->addLine(ex);
    lines->addLine(ex2);
    obstacles->addObstacle(ob);
    obstacles->addObstacle(ob2);

    std::cout << "Test Lines json encoding" << std::endl;
    std::cout << "\n" << Processor::encode_lines(*lines) << std::endl;

    std::cout << "Test Obstacles json encoding" << std::endl;
    std::cout << "\n" << Processor::encode_obstacles(*obstacles);

    std::string unit = "VISION";
    bool stat = false;
    std::string log = "Exception";
    Status* status = new Status(unit, stat, log);

    MotorBroadcast* motorBroadcast = new MotorBroadcast(143141234124);
    motorBroadcast->addLeft(10);
    motorBroadcast->addLeft(3200);
    motorBroadcast->addLeft(-0.5);

    motorBroadcast->addRight(-10);
    motorBroadcast->addRight(-3200);
    motorBroadcast->addRight(0.234);

    std::cout << "Test Status encoding" << std::endl;
    std::cout << Processor::encode_status(*status) << std::endl;

    std::cout << "Test Motorbroadcast encoding" << std::endl;
    std::cout << Processor::encode_motorbroad(*motorBroadcast) << std::endl;


    std::string motorbroad = Processor::encode_motorbroad(*motorBroadcast);

    std::cout << "Test MotorBroadcast decoding" << std::endl;
    MotorBroadcast* decodedBroadcast = Processor::decode_motorbroad(motorbroad);

    std::cout << "Motor time: " << decodedBroadcast->time << std::endl;

    for (std::vector<double>::iterator iter = decodedBroadcast->getLeft()->begin(); iter != decodedBroadcast->getLeft()->end(); ++iter) {
        std::cout << "Left value: " << *iter << std::endl;
    }

    for (std::vector<double>::iterator iter = decodedBroadcast->getRight()->begin(); iter != decodedBroadcast->getRight()->end(); ++iter) {
        std::cout << "Right value: " << *iter << std::endl;
    }
}