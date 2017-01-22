//
// Created by armin1215 on 1/14/17.
//

#ifndef CPPFRAME_PROCESSOR_H
#define CPPFRAME_PROCESSOR_H

#include <vector>
#include <deque>
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

class Interval {
public:
    double interval;

    Interval(Document& d);
    ~Interval();
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

    std::deque<Command*>* commands;

    Commands(Document& d);

    Command* remove();
    bool isEmpty();

    ~Commands();
};

class Status {
public:
    std::string unit;
    bool running;
    std::string message;

    void Serialize(Writer<StringBuffer>& writer) const;

    Status(std::string, bool status, std::string);
    Status(Document&);
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
namespace Processor {

    static std::string encode_gps(GPSMessage &to_encode) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static GPSMessage *decode_gps(std::string to_decode, bool preprocessed) {
        Document d;
        d.Parse(to_decode.c_str());
        return new GPSMessage(d, preprocessed);
    }

    static Interval *decode_interval(std::string to_decode) {
        Document d;
        d.Parse(to_decode.c_str());
        return new Interval(d);
    }

    static std::string encode_lines(Lines &to_encode) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static std::string encode_obstacles(Obstacles &to_encode) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static Commands *decode_commands(std::string to_decode) {
        Document d;
        d.Parse(to_decode.c_str());
        return new Commands(d);
    }

    static std::string encode_status(Status &to_encode) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static Status* decode_status(std::string to_decode) {
        Document d;
        d.Parse(to_decode.c_str());
        return new Status(d);
    }

    static std::string encode_motorbroad(MotorBroadcast &to_encode) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static MotorBroadcast *decode_motorbroad(std::string to_decode) {
        Document d;
        d.Parse(to_decode.c_str());
        return new MotorBroadcast(d);
    }
};



#endif //CPPFRAME_PROCESSOR_H
