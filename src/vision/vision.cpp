#include <string>
#include <map>

#include "../../include/mq.h"
#include "../../include/processor.h"
/*
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <stdint.h>
#include <stdio.h>

#include <linux/cuda.h>
*/
// ZED
#include <zed/Camera.hpp>

#include <zed/Mat.hpp>
#include <zed/Camera.hpp>
#include <zed/utils/GlobalDefine.hpp>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/gpu/gpu.hpp>

MessageHeaders headers;
ExchKeys exchKeys;

// Real GPS code

// Report status every ten publishes
int status = 10;
std::map<std::string, std::string> exchange_keys;

struct LinesPublishInfo {
    std::string exchange = exchKeys.vision_exchange;
    std::string key = exchKeys.nav_key;
    std::string header = headers.WLINES;
} linesInfo;

struct StatusPublish {
    std::string exchange = exchKeys.vision_exchange;
    std::string key = exchKeys.control_key;
    std::string header = headers.WSTATUS;
} statusInfo;

// Mutexes to protect global data

static double cameraAngle = 45.0;
struct Data {
    bool running = true;
    double interval = 0.1; // 10 per second equates to approximately 10fps, perhaps less with calculation times
    GPSMessage* gpsMessage;
} _data;

struct Mutexes {
    std::mutex running;
    std::mutex interval;
    std::mutex gps;
} _mutexes;

struct ZEDCtxt {
    sl::zed::InitParams params;
    sl::zed::Camera* zedCamera;

    std::string host;
};


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

void setGPS(GPSMessage* gpsMessage) {
    _mutexes.gps.lock();

    if (_data.gpsMessage != nullptr) {
        delete gpsMessage;
    }

    _data.gpsMessage = gpsMessage;

    _mutexes.gps.unlock();
}

GPSMessage* getGPS() {
    _mutexes.gps.lock();
    GPSMessage* gpsMessage = _data.gpsMessage;
    _mutexes.gps.unlock();
    return gpsMessage;
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


int32_t RecordFrame(ZEDCtxt* ctxt, cv::gpu::GpuMat& recFrame, sl::zed::Mat& depth)
{
    if(!ctxt->zedCamera->grab(sl::zed::SENSING_MODE::STANDARD, 1, 1, 1))
    {
        sl::zed::Mat imageView = ctxt->zedCamera->getView_gpu(sl::zed::VIEW_MODE::STEREO_SBS);

        printf("width = %08X height = %08X getWidthByte() = %08X  getDataSize() = %08X\n", imageView.width, imageView.height, imageView.getWidthByte(), imageView.getDataSize());

        int cvType = 0;

        switch(imageView.data_type)
        {
            case sl::zed::DATA_TYPE::UCHAR:
                cvType = CV_MAKE_TYPE(CV_8U, imageView.channels);
                break;
            case sl::zed::DATA_TYPE::FLOAT:
                cvType = CV_MAKE_TYPE(CV_32F, imageView.channels);
                break;
            default:
                printf("Invalid data type %08X\n", imageView.data_type);
        }

        recFrame = cv::gpu::GpuMat(imageView.height, imageView.width, cvType, imageView.data);

        depth = ctxt->zedCamera->retrieveMeasure_gpu(sl::zed::MEASURE::DEPTH);
        return 1;
    }

    return 0;
}

void ProcessFrame(ZEDCtxt* ctxt, cv::gpu::GpuMat& frame, cv::vector<cv::Vec4i>& outLines)
{
    uint32_t rows = frame.rows;
    uint32_t cols = frame.cols/2;

    cv::gpu::GpuMat cvImageViewGray(rows, cols, CV_8UC1);

    // Currently just going to grab the right image until we can accurately combine the images

    cv::gpu::GpuMat half = frame.colRange(0, frame.cols/2);
    cv::gpu::GpuMat afterGaus;

    cv::gpu::cvtColor(half, cvImageViewGray, CV_BGRA2GRAY);

    uint32_t gausCount = 1;

    for(uint32_t i = 0; i < gausCount; i++)
    {
        cv::gpu::GaussianBlur(half, afterGaus, cv::Size(21,21), 0, 0, cv::BORDER_DEFAULT);
    }

    cv::gpu::GpuMat cvEdges(rows, cols, CV_8UC3);

    for(uint32_t i = 0; i < 7; i++)
    {
        cv::gpu::Canny(afterGaus, cvEdges, 30, 90);
    }

    cv::Mat cdst(rows, cols, CV_8UC3);
    cv::gpu::GpuMat houghOut;

    cv::gpu::HoughLinesBuf hough_buffer;

    //printf("rows = %d cols = %d\n", image.rows, image.cols);
    cv::gpu::HoughLinesP(cvEdges, houghOut, hough_buffer, 1, CV_PI/180, 25, 5, 50);

    // Going to find runs of points in a certain pixel distance from eachother
    // in the x coordinate plane then we'll derive a single point using the average of 
    // the x coordinates and either the longest or a derived direction and length
    uint8_t runLengthY = 60;
    uint8_t runLengthX = 50;
    uint32_t curIndex = 0;

    cv::vector<cv::Vec4i> houghLines;
    cv::vector<cv::Vec4i> combLines;
    cv::vector<uint32_t> indxs;

    houghLines.resize(houghOut.cols);

    cv::Mat houghDl(1, houghOut.cols, CV_8UC3, &houghLines[0]);
    houghOut.download(houghDl);

    while(true)
    {
        cv::Vec4i baseLine = houghLines[curIndex];

        for(uint32_t j = 0; j < indxs.size(); j++)
        {
            if(indxs[j] == curIndex)
            {
                curIndex++;
                continue;
            }
        
        }

        indxs.push_back(curIndex);

        uint32_t totX = 0;
        uint32_t totXDst = 0;
        uint32_t numNodes = 0;

        int32_t maxY = 0;

        for(uint32_t i = 0; i < houghLines.size(); i++)
        {
            int32_t deltaY = houghLines[i][1] - baseLine[1];
            int32_t deltaX = houghLines[i][0] - baseLine[0];
            // printf("deltaX = %d   abs(deltaX) = %d   deltaY = %d   abs(deltaY) = %d\n", deltaX, abs(deltaX), deltaY, abs(deltaY));
            for(uint32_t j = 0; j < indxs.size(); j++)
            {
                // if we've already seen this index just move along
                if(indxs[j] == i)
                    continue;
            }
           // if(abs(deltaY) <= runLengthY && 
            if(abs(deltaX) <= runLengthX)
            {
                numNodes++;
                totX += houghLines[i][0];
                totXDst += houghLines[i][2];

                if(maxY < houghLines[i][3])
                    maxY = houghLines[i][3];

                indxs.push_back(i);
            }
        }

        if(numNodes > 0)
        {
            cv::Vec4i cLine;

            cLine[0] = baseLine[0];//totX / numNodes;
            cLine[1] = baseLine[1];

            cLine[2] = totXDst / numNodes;
            cLine[3] = maxY;
            // printf("maxY = %d\n", maxY);

            

            combLines.push_back(cLine);
            outLines.push_back(cLine);
        }

        if(curIndex == houghLines.size())
            break;

        curIndex++;
    }

/* TEST CODE, REMOVE BEFORE USE */
/*
    for(uint32_t i = 0; i < combLines.size(); i++)
    {
        cv::Vec4i l = combLines[i];
        cv::line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,0,255), 3, CV_AA);

        printf("point x = %d y = %d     dest x= %d y = %d\n", l[0], l[1], l[2], l[3]);

    }

    cv::imshow("test", cdst);
    cv::waitKey(1);
*/
/* TEST CODE */

}


/**
 * This is where all Vision computations will take place. Check data fields for changes on loop. If state of the
 * robot has changed respond accordingly.
 *
 * Set up a loop to publish the location, acceleration, velocity, etc. once every tenth of a second.
 * TODO: Periodically send a status update to the Control unit
 * @param host string name of the host with the ip:port number as necessary
 */
void vis_publisher(ZEDCtxt* ctxt) {
    std::string exchange = exchKeys.gps_exchange;
    std::string key = exchKeys.gps_key;

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

        sl::zed::Mat depthMat;
        cv::gpu::GpuMat gpuFrame;
        cv::vector<cv::Vec4i> lines;

        if(RecordFrame(ctxt, gpuFrame, depthMat))
            ProcessFrame(ctxt, gpuFrame, lines);
        else
            continue;

        // TODO: populate lat/lon/time
        Lines* obstLines = new Lines(0, 0, 0);

        for(uint32_t i = 0; i < lines.size(); i++)
        {
            cv::Vec4i l = lines[i];
            Line obLine;

            obLine.beginX = l[0];
            obLine.endX = l[2];

            sl::uchar3 d = depthMat.getValue(l[0], l[1]);

            // Need to update the y coordinate with the depth map

            printf("depth @ x %d  y  %d: %d %d %d", l[0], l[1], d.c1, d.c2, d.c3);

            // depth map returns in millimeters we want 10cm increments so /100
            obLine.beginY = cos(cameraAngle)* (d.c1/ 100);
            d = depthMat.getValue(l[2], l[3]);
            obLine.endY = cos(cameraAngle) * (d.c1/100);

            obstLines->addLine(obLine);
        }

        // Get gps message here and convert JSON -> GPSMessage -> std::string
        std::string message = "my_message";
        send_message(connection, Processor::encode_lines(*obstLines), linesInfo.header, linesInfo.exchange, linesInfo.key);
        std::cout << _iterations << std::endl;

        if (_iterations % status == 0) {
            Status* status = new Status(exchKeys.VISION, getRunning(), "normal");
            std::string status_mess = Processor::encode_status(*status);
            send_message(connection, status_mess, statusInfo.header, statusInfo.exchange, statusInfo.key);
            _iterations = 0;
        }

        std::chrono::duration<double> interval(getInterval());

        auto end = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(interval - (end - start));
    }

    std::string closing = "closing";
    close_message(connection, closing, exchange, key);
}

// Listen for incoming information like commands from Control or requests from other components
void vis_subscriber(std::string host) {
    start.lock();
    MessageHeaders headers1;

    std::string queue = exchKeys.vision_sub;

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
//        std::cout << getMessage() << std::endl;
//        setMessage();
        if (header == headers1.WGPSFRAME) {
            GPSMessage* gpsMessage = Processor::decode_gps(message.message(), true);
            setGPS(gpsMessage);
            std::cout << "decode_gps" << std::endl;
        } else if (header == headers1.WINTERVAL) {
            Interval* interval = Processor::decode_interval(message.message());
            setInterval(interval->interval);
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

    ZEDCtxt* z_ctxt = new ZEDCtxt();

    z_ctxt->params.verbose = true;
    z_ctxt->params.mode = sl::zed::MODE::QUALITY;
    z_ctxt->params.unit = sl::zed::UNIT::MILLIMETER;

    z_ctxt->zedCamera = new sl::zed::Camera(sl::zed::ZEDResolution_mode::HD720);

    sl::zed::ERRCODE err = z_ctxt->zedCamera->init(z_ctxt->params);

    if(err !=  sl::zed::SUCCESS) {
        std::cout << "Unable to init the ZED:" << errcode2str(err) << std::endl;
        delete z_ctxt->zedCamera;
        return 1;
    }

    exchange_keys.insert({exchKeys.gps_exchange, exchKeys.gps_key});
    exchange_keys.insert({exchKeys.control_exchange, exchKeys.vision_key});
    exchange_keys.insert({exchKeys.nav_exchange, exchKeys.vision_key});

    std::thread sub(vis_subscriber, host);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::thread pub(vis_publisher, z_ctxt);

    sub.join();
    pub.join();

}
