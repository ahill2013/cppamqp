//
// Created by armin1215 on 1/14/17.
//

#ifndef CPPFRAME_PROCESSOR_H
#define CPPFRAME_PROCESSOR_H

#include <vector>
#include <deque>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/pointer.h>
#include <stdint.h>
//using namespace rapidjson;

class GPSMessage {
public:
    double lat;
    double lon;
    double linvel;
    double linacc;
    double angvel;
    double angacc;
    uint64_t time;
    bool ins;
    double heading;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    GPSMessage(double lat, double lon, double linvel, double angvel, uint64_t numTime, double heading);
    GPSMessage(rapidjson::Document& d);
    ~GPSMessage();
};

class Interval {
public:
    double interval;

    Interval(rapidjson::Document& d);
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
    uint64_t time;
    std::vector<Obstacle>* obstacles;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    void addObstacle(Obstacle&);

    Obstacles(double lat, double lon, uint64_t time);
    ~Obstacles();
};


class Lines {
public:
    double lat;
    double lon;
    uint64_t time;
    std::vector<Line>* lines;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    void addLine(Line&);

    Lines(double lat, double lon, uint64_t time);
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

    Command(double linvel, double angvel, double startang, double endlat, double endlon, double duration);
    Command(const rapidjson::Value& val);
    ~Command();
};

class Commands {
public:
    double startLat;
    double startLon;

    std::deque<Command*>* commands;

    Commands(rapidjson::Document& d);

    Command* remove();
    bool isEmpty();

    ~Commands();
};

class Status {
public:
    std::string unit;
    bool running;
    std::string message;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    Status(std::string, bool status, std::string);
    Status(rapidjson::Document&);
    ~Status();
};

class MotorBroadcast {
public:
    uint64_t time;


    void addLeft(double);
    void addRight(double);

    std::vector<double>* getLeft();
    std::vector<double>* getRight();

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    MotorBroadcast(uint64_t);
    MotorBroadcast(rapidjson::Document &d);
    ~MotorBroadcast();
private:
    std::vector<double>* left;
    std::vector<double>* right;
};



// Put all message types (as structs) in here, include ways to send and parse them)
namespace Processor {

    static std::string encode_gps(GPSMessage &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static GPSMessage *decode_gps(std::string to_decode) {
        rapidjson::Document d;
        d.Parse(to_decode.c_str());
        return new GPSMessage(d);
    }

    static Interval *decode_interval(std::string to_decode) {
        rapidjson::Document d;
        d.Parse(to_decode.c_str());
        return new Interval(d);
    }

    static std::string encode_lines(Lines &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static std::string encode_obstacles(Obstacles &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static Commands *decode_commands(std::string to_decode) {
        rapidjson::Document d;
        d.Parse(to_decode.c_str());
        return new Commands(d);
    }

    static std::string encode_status(Status &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static Status* decode_status(std::string to_decode) {
        rapidjson::Document d;
        d.Parse(to_decode.c_str());
        return new Status(d);
    }

    static std::string encode_motorbroad(MotorBroadcast &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    static MotorBroadcast *decode_motorbroad(std::string to_decode) {
        rapidjson::Document d;
        d.Parse(to_decode.c_str());
        return new MotorBroadcast(d);
    }
};



#endif //CPPFRAME_PROCESSOR_H
