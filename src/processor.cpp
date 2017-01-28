//
// Created by armin1215 on 1/14/17.
//

#include "../include/processor.h"
#include "iostream"

using namespace rapidjson;

// Simple writer
void GPSMessage::Serialize(Writer<StringBuffer> &writer) const {
    writer.StartObject();
    writer.String("lat");
    writer.Double(lat);

    writer.String("lon");
    writer.Double(lon);

    writer.String("time");
    writer.String(time.c_str(), static_cast<SizeType>(time.length()));

    writer.EndObject();
}

GPSMessage::GPSMessage(Document &d, bool preprocessed) {
    if (preprocessed) {
        std::cout << "Preprocessed" << std::endl;

        lat = GetValueByPointer(d, "/lat")->GetDouble();
        lon = GetValueByPointer(d, "/lon")->GetDouble();
        time = GetValueByPointer(d, "/time")->GetString();
    } else {
        lat = GetValueByPointer(d, "/data/lat")->GetDouble();
        lon = GetValueByPointer(d, "/data/lon")->GetDouble();
        time = d["time"].GetString();
    }

}

GPSMessage::~GPSMessage() = default;

////////////////////////////////////////////////////////////////////////

Interval::Interval(Document& d) {
    interval = d["interval"].GetDouble();
}

Interval::~Interval() = default;

////////////////////////////////////////////////////////////////////////


Lines::Lines(double lat, double lon, uint64_t time) {
    this->lat = lat;
    this->lon = lon;
    this->time = time;
    lines = new std::vector<Line>();
}

void Lines::addLine(Line& line) {
    lines->push_back(line);
}

void Lines::Serialize(Writer<StringBuffer> &writer) const {
    Document jsonDoc;
    jsonDoc.SetObject();
    Document::AllocatorType& allocator = jsonDoc.GetAllocator();

    Value linesArray(kArrayType);

    Value latValue;
    latValue.SetDouble(lat);

    Value lonValue;
    lonValue.SetDouble(lon);

    Value timeValue;
    timeValue.SetUint64(time);

    for (Line& line : *lines) {
        Value lineVal;
        lineVal.SetObject();
        lineVal.AddMember("beginX", line.beginX, allocator);
        lineVal.AddMember("beginY", line.beginY, allocator);
        lineVal.AddMember("endX", line.endX, allocator);
        lineVal.AddMember("endY", line.endY, allocator);
        linesArray.PushBack(lineVal, allocator);
    }

    jsonDoc.AddMember("lat", latValue, allocator);
    jsonDoc.AddMember("lon", lonValue, allocator);
    jsonDoc.AddMember("time", timeValue, allocator);
    jsonDoc.AddMember("lines", linesArray, allocator);

    jsonDoc.Accept(writer);
}

Lines::~Lines() {
    delete lines;
}

///////////////////////////////////////////////////////////////////////////
Obstacles::Obstacles(double lat, double lon, uint64_t time) {
    this->lat = lat;
    this->lon = lon;
    this->time = time;
    obstacles = new std::vector<Obstacle>();
}

void Obstacles::Serialize(Writer<StringBuffer> &writer) const {
    Document jsonDoc;
    jsonDoc.SetObject();
    Document::AllocatorType& allocator = jsonDoc.GetAllocator();

    Value obsArray(kArrayType);

    Value latValue;
    latValue.SetDouble(lat);

    Value lonValue;
    lonValue.SetDouble(lon);

    Value timeValue;
    timeValue.SetUint64(time);

    for (Obstacle& obstacle : *obstacles) {
        Value obsVal;
        obsVal.SetObject();
        obsVal.AddMember("x", obstacle.x, allocator);
        obsVal.AddMember("y", obstacle.y, allocator);
        obsVal.AddMember("type", obstacle.type, allocator);
        obsVal.AddMember("radius", obstacle.radius, allocator);
        obsArray.PushBack(obsVal, allocator);
    }

    jsonDoc.AddMember("lat", latValue, allocator);
    jsonDoc.AddMember("lon", lonValue, allocator);
    jsonDoc.AddMember("time", timeValue, allocator);
    jsonDoc.AddMember("obstacles", obsArray, allocator);

    jsonDoc.Accept(writer);
}

void Obstacles::addObstacle(Obstacle& obstacle) {
    obstacles->push_back(obstacle);
}


Obstacles::~Obstacles() {
    delete obstacles;
}
//////////////////////////////////////////////////////////////////////////
Command::Command(const Value& val) {
    startang = val["startang"].GetDouble();
    linvel = val["linvel"].GetDouble();
    angvel = val["angvel"].GetDouble();
    duration = val["duration"].GetUint();
    endLat = val["endLat"].GetDouble();
    endLon = val["endLon"].GetDouble();

}

Command::~Command() = default;

Commands::Commands(Document &d) {

    Value& lat = d["startLat"];
    startLat = lat.GetDouble();

    Value& lon = d["startLon"];
    startLon = lon.GetDouble();

    commands = new std::deque<Command*>();

    Value& arrayized = d["commands"];

    for (Value::ConstValueIterator itr = arrayized.Begin(); itr != arrayized.End(); ++itr) {
        Command* comm = new Command(*itr);
        commands->push_back(comm);
    }
}


bool Commands::isEmpty() {
    return commands->empty();
}

Command* Commands::remove() {
    Command* command = commands->front();
    commands->pop_front();
    return command;
}

Commands::~Commands() {
    delete commands;
}


/////////////////////////////////////////////////////////////////////////
Status::Status(std::string comp, bool status, std::string logmessage) {
    unit = comp;
    running = status;
    message = logmessage;
}

Status::Status(Document& d) {
    Value& u = d["unit"];
    Value& run = d["running"];
    Value& mess = d["message"];

    unit = u.GetString();
    running = run.GetBool();
    message = mess.GetString();
}

void Status::Serialize(Writer<StringBuffer> &writer) const {
    Document jsonDoc;
    jsonDoc.SetObject();
    Document::AllocatorType& allocator = jsonDoc.GetAllocator();

    Value softwareUnit;
    softwareUnit.SetString(unit.c_str(), allocator);

    Value  logMessage;
    logMessage.SetString(message.c_str(), allocator);

    jsonDoc.AddMember("unit", softwareUnit, allocator);
    jsonDoc.AddMember("running", running, allocator);
    jsonDoc.AddMember("message", logMessage, allocator);

    jsonDoc.Accept(writer);

}

Status::~Status() = default;


/////////////////////////////////////////////////////////////////////////
MotorBroadcast::MotorBroadcast(uint64_t t) {
    time = t;
    left = new std::vector<double>();
    right = new std::vector<double>();
}

MotorBroadcast::MotorBroadcast(Document& d) {

    left = new std::vector<double>();
    right = new std::vector<double>();

    Value& t = d["time"];
    time = t.GetUint64();

    Value& leftArray = d["left"];
    Value& rightArray = d["right"];

    for (SizeType i  = 0; i < leftArray.Size(); i++) {
        const Value& encoder_val = leftArray[i];
        left->push_back(encoder_val.GetDouble());
    }

    for (Value::ConstValueIterator itr = rightArray.Begin(); itr != rightArray.End(); ++itr) {
        double encoder_val = (*itr).GetDouble();
        right->push_back(encoder_val);
    }
}

void MotorBroadcast::addLeft(double encoder_val) {
    left->push_back(encoder_val);
}

void MotorBroadcast::addRight(double encoder_val) {
    right->push_back(encoder_val);
}

std::vector<double>* MotorBroadcast::getLeft() {
    return left;
}

std::vector<double>* MotorBroadcast::getRight() {
    return right;
}

void MotorBroadcast::Serialize(Writer<StringBuffer> &writer) const {
    Document jsonDoc;
    jsonDoc.SetObject();
    Document::AllocatorType& allocator = jsonDoc.GetAllocator();

    Value leftArray(kArrayType);
    Value rightArray(kArrayType);

    Value timeValue;
    timeValue.SetUint64(time);

    for (std::vector<double>::iterator it = left->begin(); it != left->end(); ++it) {
        Value encoder_val;
        encoder_val.SetDouble(*it);
        leftArray.PushBack(encoder_val, allocator);
    }

    for (std::vector<double>::iterator it = right->begin(); it != right->end(); ++it) {
        Value encoder_val;
        encoder_val.SetDouble(*it);
        rightArray.PushBack(encoder_val, allocator);
    }

    jsonDoc.AddMember("time", timeValue, allocator);
    jsonDoc.AddMember("left", leftArray, allocator);
    jsonDoc.AddMember("right", rightArray, allocator);

    jsonDoc.Accept(writer);

}

MotorBroadcast::~MotorBroadcast() {
    delete left;
    delete right;
}

/////////////////////////////////////////////////////////////////////////
