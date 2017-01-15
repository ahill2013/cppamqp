//
// Created by armin1215 on 1/14/17.
//

#ifndef CPPFRAME_PROCESSOR_H
#define CPPFRAME_PROCESSOR_H

#include <vector>
#include "../lib/rapidjson/document.h"
#include "../lib/rapidjson/writer.h"
#include "../lib/rapidjson/stringbuffer.h"
#include "../lib/rapidjson/pointer.h"

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

class Obstacles {
public:
    double lat;
    double lon;
    unsigned long time;
    std::vector<Obstacle>* obstacles;

    void Serialize(Writer<StringBuffer>& writer) const;

    void addObstacle(Obstacle&);

    Obstacles(double lat, double lon, unsigned long time);
    ~Obstacles();
};


class Lines {
public:
    double lat;
    double lon;
    unsigned long time;
    std::vector<Line>* lines;

    void Serialize(Writer<StringBuffer>& writer) const;

    void addLine(Line&);

    Lines(double lat, double lon, unsigned long time);
    ~Lines();
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

class Status {
public:
    std::string unit;
    bool running;
    std::string message;

    void Serialize(Writer<StringBuffer>& writer) const;

    Status(std::string, bool status, std::string);
    ~Status();
};

class MotorBroadcast {
public:
    unsigned long time;


    void addLeft(double);
    void addRight(double);

    std::vector<double>* getLeft();
    std::vector<double>* getRight();

    void Serialize(Writer<StringBuffer>& writer) const;

    MotorBroadcast(unsigned long);
    MotorBroadcast(Document &d);
    ~MotorBroadcast();
private:
    std::vector<double>* left;
    std::vector<double>* right;
};



// Put all message types (as structs) in here, include ways to send and parse them)
class Processor {
public:
    // Repeat one of these per message
    std::string encode_gps(GPSMessage& to_encode);
    GPSMessage* decode_gps(std::string to_decode, bool);

    std::string encode_lines(Lines& to_encode);

    std::string encode_obstacles(Obstacles& to_encode);

    Commands* decode_commands(std::string to_decode);

    std::string encode_status(Status& to_encode);

    std::string encode_motorbroad(MotorBroadcast& to_encode);
    MotorBroadcast* decode_motorbroad(std::string to_decode);
};



#endif //CPPFRAME_PROCESSOR_H
