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

/**
 * @brief Details of information gathered by an ins and gps to be broadcast to other components
 */
class GPSMessage {
public:
    double lat; // Latitude
    double lon; // Longitude
    double linvel;  // Linear velocity
    double linacc;  // Linear acceleration (not sent currently)
    double angvel;  // Angular velocity
    double angacc;  // Angular acceleration (not in use)
    uint64_t time;  // Time in milliseconds since gps epoch
    bool ins;
    double heading; // Heading of the robot

    /**
     * @brief Method to serialize GPSMessage
     */
    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    /**
     * @brief GPSMessage constructor fill in with information that should be sent
     */
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

/**
 * @brief Single obstacle with an x, y coordinate and a radius
 */
struct Obstacle {
    double x;
    double y;
    double radius;

    // REDFLAG, BLUEFLAG, FENCE, GENERIC
    int type;
};

/**
 * @brief Single line with a beginning and ending coordinate
 */
struct Line {
    double beginX;
    double beginY;
    double endX;
    double endY;
};

/**
 * @brief List of circular obstacles
 */
class Obstacles {
public:
    double lat;
    double lon;
    uint64_t time;
    std::vector<Obstacle>* obstacles;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    void addObstacle(Obstacle&);
    int size();
    Obstacles(double lat, double lon, uint64_t time);
    ~Obstacles();
};

/**
 * @brief List of lines detected by the ZED camera
 */
class Lines {
public:
    double lat;
    double lon;
    uint64_t time;
    std::vector<Line*>* lines;

    void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

    void addLine(Line*);

    Lines(double lat, double lon, uint64_t time);
    ~Lines();
};

/**
 * @brief Single command to be sent
 */
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

/**
 * @brief List of commands to be executed by motor control
 */
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


/**
 * @brief namespace for all encode and decode methods for serializing to JSON and deserializing from JSON
 */
namespace Processor {

    /**
     * @brief Encode a gps message as json
     * @param to_encode GPSMessage to encode
     * @return RapidJSON document as string
     */
    static std::string encode_gps(GPSMessage &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    /**
     * Decode a gps message from a string
     * @param to_decode RapidJSON document as string
     * @return decoded GPSMessage
     */
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

    /**
     * Encode list of lines to JSON string
     * @param to_encode list of lines to encode
     * @return RapidJSON document as string
     */
    static std::string encode_lines(Lines &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    /**
     * Encode list of obstacles as JSON string
     * @param to_encode list of obstacles to encode
     * @return RapidJSON document as string
     */
    static std::string encode_obstacles(Obstacles &to_encode) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        to_encode.Serialize(writer);
        return buffer.GetString();
    }

    /**
     * Decode a JSON string into a list of commands
     * @param to_decode json string to decode
     * @return Commands object
     */
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
