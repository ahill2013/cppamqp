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
static std::string pathName = "/home/ubuntu/visiontest/";
static uint64_t numFrames = 0;
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

//#define TEST


#ifdef TEST
cv::VideoCapture capture("/home/ubuntu/45_into_sun_polar.mp4");
#endif

int32_t RecordFrame(ZEDCtxt* ctxt, cv::gpu::GpuMat& recFrame, cv::gpu::GpuMat& depth)//sl::zed::Mat& depth)
{
#ifndef TEST
    if(!ctxt->zedCamera->grab(sl::zed::SENSING_MODE::STANDARD, 1, 1, 1))
    {
        sl::zed::Mat imageView = ctxt->zedCamera->getView_gpu(sl::zed::VIEW_MODE::STEREO_SBS);
        sl::zed::Mat depthView;

        printf("width = %08X height = %08X getWidthByte() = %08X  getDataSize() = %08X\n", imageView.width, imageView.height, imageView.getWidthByte(), imageView.getDataSize());

        int cvType = 0;

        switch(imageView.data_type)
        {
            case sl::zed::DATA_TYPE::UCHAR:
                cvType = CV_MAKE_TYPE(CV_8U, imageView.channels);
                break;
            case sl::zed::DATA_TYPE::FLOAT:
                cvType = CV_MAKE_TYPE(CV_32F,3);// imageView.channels);
                break;
            default:
                printf("Invalid data type %08X\n", imageView.data_type);
        }

        recFrame = cv::gpu::GpuMat(imageView.height, imageView.width, cvType, imageView.data);

        depthView = ctxt->zedCamera->retrieveMeasure_gpu(sl::zed::MEASURE::DEPTH);
        switch(imageView.data_type)
        {
            case sl::zed::DATA_TYPE::UCHAR:
                cvType = CV_MAKE_TYPE(CV_8U, depthView.channels);
                break;
            case sl::zed::DATA_TYPE::FLOAT:
                cvType = CV_MAKE_TYPE(CV_32F, 3);
                break;
            default:
                printf("Invalid Depth data type %d\n", depthView.data_type);
        }

        depth = cv::gpu::GpuMat(depthView.height, depthView.width, cvType, depthView.data);
        
        return 1;
    }
#endif

#ifdef TEST

    cv::Mat image;

    if(!capture.isOpened())
    {
        printf("Failed to open the video\n");
        return 0;
    }

    if(!capture.read(image))
        return -5;

    recFrame.upload(image);


    return 1;

#endif

    return 0;
}

//#define CPU

#ifdef CPU
cv::gpu::GpuMat HomomorphicFilter(cv::gpu::GpuMat& input)
{
    cv::Mat dlOrig;
    cv::Mat ycrImg;
    cv::Mat yChan;
    cv::Mat output;
    cv::gpu::GpuMat gpuOut;

    std::vector<cv::Mat> imgParts;
    std::vector<cv::Mat> parts2;

    input.download(dlOrig);

    int m1 = cv::getOptimalDFTSize(input.rows);
    int n1 = cv::getOptimalDFTSize(input.cols);

    cv::cvtColor(dlOrig, ycrImg, CV_RGB2YCrCb);

    cv::split(dlOrig, imgParts);

    imgParts[0].convertTo(yChan, CV_32F);

    log(yChan, yChan);

    cv::dft(yChan, yChan);
    cv::Laplacian(yChan, yChan, CV_32F, 3);
    cv::dft(yChan, yChan, cv::DFT_INVERSE|cv::DFT_REAL_OUTPUT);

    exp(yChan, yChan);

    cv::split(yChan, parts2);
    parts2[0].convertTo(imgParts[0], CV_8U);

    cv::merge(imgParts, output);

    cv::threshold(output, output, 215, 220, CV_THRESH_BINARY);
#ifdef TEST
    cv::imwrite(pathName + "afterhomomorphic_" + std::to_string(numFrames) + ".jpg" , output);
#endif
    gpuOut.upload(output);

    return gpuOut;
}
#endif

#ifndef CPU
cv::gpu::GpuMat HomomorphicFilter(cv::gpu::GpuMat& input)
{
    cv::gpu::GpuMat output(input.size(), CV_32FC4);
    cv::gpu::GpuMat outGray;

    cv::gpu::GpuMat tmp;

    cv::gpu::GpuMat ycrImg;//(input.size(), CV_32FC3);
    cv::gpu::GpuMat yOneChan(input.size(), CV_32FC1);

    //std::vector<cv::gpu::GpuMat> imgParts;
    //std::vector<cv::gpu::GpuMat> parts2;

    cv::gpu::GpuMat imgParts[3];
    cv::gpu::GpuMat parts2[3];

    int m1 = cv::getOptimalDFTSize( input.rows );
    int n1 = cv::getOptimalDFTSize( input.cols );

    cv::gpu::cvtColor(input, ycrImg, CV_BGR2YCrCb);

#ifdef TEST
    cv::Mat dl;

    ycrImg.download(dl);

    cv::imwrite(pathName + "ycr_" + std::to_string(numFrames) + ".jpg", dl);

    printf("before split\n");
#endif
    cv::gpu::split(input, imgParts);

#ifdef TEST
    printf("after split num channels = %d\n", imgParts.size());
#endif

    cv::gpu::GpuMat yChan = imgParts[0];
    imgParts[0].copyTo(yChan);

    cv::gpu::log(yChan, yChan);

#ifdef TEST
    printf("before dft n1 = %d  m1 = %d ychan channels = %d\n", n1, m1, yChan.channels());
#endif
    cv::gpu::dft(yOneChan, yChan, cv::Size(n1, m1), 0);

#ifdef TEST
    printf("yOneChan after first dft channels = %d\n", yOneChan.channels());
#endif

    yChan = yOneChan;
#ifdef TEST
    printf("before laplacian\n");
    printf("ychan channels = %d\n", yChan.channels());
#endif
    cv::gpu::Laplacian(yChan, yChan, CV_32FC1, 3);
#ifdef TEST
    printf("before inverse dft yChan channels = %d\n", yChan.channels());
#endif
    cv::gpu::dft(yOneChan, yChan, cv::Size(n1, m1), cv::DFT_INVERSE);//|cv::DFT_REAL_OUTPUT);
    //cv::gpu::split(yChan, dftFix);
    yChan = yOneChan;

#ifdef TEST
    printf("yOneChan after second dft num chans = %d\n", yOneChan.channels());
#endif
    cv::gpu::exp(yChan, yChan);

    cv::gpu::split(yChan, parts2);

    parts2[0].convertTo(imgParts[0], CV_8UC1);//imgParts[0] = parts2[0];

#ifdef TEST
    printf("parts2 depth = %d    imgParts[0] depth = %d\n", parts2[0].depth(), imgParts[0].depth());
    //parts2[0].copyTo(imgParts[0]);

    // printf("parts2[0] = %d\n", parts2[0].depth());

    printf("imgParts[0].depth = %d  channels=%d\n", imgParts[0].depth(), imgParts[0].channels());
    printf("imgParts[1].depth = %d  channels=%d\n", imgParts[1].depth(), imgParts[1].channels());

    cv::Size tmpsz = imgParts[0].size();

    printf("tmpsz[0] = %d   tmpsz[1] = %d\n", tmpsz.width, tmpsz.height);

    tmpsz = imgParts[1].size();

    printf("w = %d h = %d\n", tmpsz.width, tmpsz.height);
    
    tmpsz = imgParts[2].size();

    printf("w = %d h = %d\n", tmpsz.width, tmpsz.height);

    for(uint32_t i = 0; i < 1;i++)//imgParts.size(); i++)
    {
        printf("parts %d  channels = %d  type = %d width = %d height = %d\n", i, imgParts[i].channels(), imgParts[i].type(), imgParts[i].size().width, imgParts[i].size().height);
        //cv::gpu::threshold(imgParts[i], imgParts[i], 150, 250,CV_THRESH_BINARY);//215, 220, CV_THRESH_BINARY);
    }

    printf("before merge\n");
#endif
    cv::gpu::merge(imgParts, 3, output);

#ifdef TEST
    output.download(dl);
    cv::imwrite(pathName + "aftermerge_" + std::to_string(numFrames) + ".jpg", dl);

    //cv::gpu::cvtColor(output,output, CV_YCrCb2BGR);

    output.download(dl);
    cv::imwrite(pathName + "afterhomomorphic_" + std::to_string(numFrames) + ".jpg" , dl);

    printf("before thresh  channels = %d   type = %d\n", output.channels(), output.type());
    //cv::gpu::threshold(output, output, 215, 220, CV_THRESH_BINARY);

#endif
    return output;
}
#endif

void ProcessFrame(ZEDCtxt* ctxt, cv::gpu::GpuMat& frame, cv::vector<cv::Vec4i>& outLines)
{
    uint32_t rows = frame.rows;
    uint32_t cols = frame.cols/2;

    cv::Mat dl;

    cv::gpu::GpuMat cvImageViewGray(rows, cols, CV_8UC1);

    // Currently just going to grab the right image until we can accurately combine the images

    cv::gpu::GpuMat half;// = frame.colRange(0, frame.cols/2);
    cv::gpu::GpuMat afterGaus;

    //frame.download(dl);

    //cv::imwrite(pathName + "orig_" + std::to_string(numFrames) + ".jpg", dl);

    cv::gpu::GpuMat homomorphicOut = HomomorphicFilter(frame);

    //homomorphicOut.copyTo(cvImageViewGray);

    //printf("homomorphic out channels = %d\n", homomorphicOut.channels());
    cv::gpu::cvtColor(homomorphicOut, cvImageViewGray, CV_BGR2GRAY);

#ifdef TEST
    cvImageViewGray.download(dl);
    cv::imwrite(pathName + "gray_" + std::to_string(numFrames) + ".jpg" , dl);
#endif 

    uint32_t gausCount = 1;

    afterGaus = cvImageViewGray;

    for(uint32_t i = 0; i < gausCount; i++)
    {
        cv::gpu::GaussianBlur(afterGaus, afterGaus, cv::Size(11,11), 0, 0, cv::BORDER_DEFAULT);
    }

#ifdef TEST
    afterGaus.download(dl);
    cv::imwrite(pathName + "gaussian_" + std::to_string(numFrames) + ".jpg" , dl);
#endif

    cv::gpu::GpuMat cvEdges(rows, cols, CV_8UC3);

    for(uint32_t i = 0; i < 7; i++)
    {
        cv::gpu::Canny(afterGaus, cvEdges, 30, 90);
    }

    cv::Mat cdst(rows, cols, CV_8UC3);
    cv::gpu::GpuMat houghOut;

    cv::gpu::HoughLinesBuf hough_buffer;

    //printf("rows = %d cols = %d\n", image.rows, image.cols);
    cv::gpu::HoughLinesP(cvEdges, houghOut, hough_buffer, 1, CV_PI/180, 25, 50, 70);

    cv::vector<cv::Vec4i> houghLines;
    cv::vector<cv::Vec4i> combLines;
    cv::vector<uint32_t> indxs;

    houghLines.resize(houghOut.cols);

    cv::Mat houghDl(1, houghOut.cols, CV_32SC4, &houghLines[0]);
    houghOut.download(houghDl);

#ifdef TEST
    printf("lines = %d\n", houghLines.size());
#endif 

    /*for(uint32_t i = 0; i < houghLines.size(); i++)
    {
#ifdef TEST
        printf("0 = %d  1 = %d  2 = %d  3 = %d\n", houghLines[i][0], houghLines[i][1], houghLines[i][2], houghLines[i][3]);
#endif
        outLines.push_back(houghLines[i]);
    }*/

    outLines = houghLines;
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

//    start.lock();
//    start.unlock();

    // Turn this into a while(true) loop to keep posting messages
    while (_iterations < 20) {
        std::cout << "I'm an iteration" << _iterations << std::endl;
        _iterations++;
        auto start = std::chrono::high_resolution_clock::now();

        //sl::zed::Mat depthMat;
        cv::gpu::GpuMat gpuFrame;
        cv::gpu::GpuMat depthMat;
        cv::Mat dlDepth;
        cv::vector<cv::Vec4i> lines;

        uint32_t cols = gpuFrame.cols;
        uint32_t rows = gpuFrame.rows;
        uint32_t halfCols = gpuFrame.cols/2;

        double theta1 = 0.0;
        double endTheta1 = 0.0;

        if(RecordFrame(ctxt, gpuFrame, depthMat))
            ProcessFrame(ctxt, gpuFrame, lines);
        else {
            std::cout << "Failed to record " << _iterations << std::endl;
            continue;
        }
        // TODO: populate lat/lon/time
        Lines* obstLines = new Lines(0, 0, 0);

        depthMat.download(dlDepth);

#ifdef TEST
        cv::Mat outMat;
#endif

        for(uint32_t i = 0; i < lines.size(); i++)
        {
            cv::Vec4i l = lines[i];
            Line obLine;

            cv::Vec4i clean;
            uint32_t realX = 0;
            uint32_t realY = 0;

            double beginActR = 0.0;
            double endActR = 0.0;

            //printf("line %d\n", i);

            if(l[0] > halfCols)
            {
                clean[0] = (l[0]-halfCols)-20;
                clean[2] = (l[2]-halfCols)-20;
            }

            clean[1] = l[1];
            clean[3] = l[3];

#ifdef TEST
            printf("clean 0 = %d   1 = %d    2 = %d   3 = %d\n", clean[0], clean[1], clean[2], clean[3]);
            cv::line(outMat, cv::Point(clean[0], clean[1]), cv::Point(clean[2], clean[3]), cv::Scalar(150,150,150), 3, CV_AA);
#endif 
            theta1 = atan((rows-clean[1])/(clean[0]-halfCols));
            endTheta1 = atan((rows-clean[3])/(clean[2]-halfCols));

            // obLine.beginX = clean[0];
            // obLine.endX = clean[2];

#ifndef TEST
            cv::Vec3b p1 = dlDepth.at<cv::Vec3b>(l[0], l[1]);//depthMat.getValue(l[0], l[1]);
            cv::Vec3b p2 = dlDepth.at<cv::Vec3b>(l[2], l[3]);//depthMat.getValue(l[2], l[3]);

            // Need to update the y coordinate with the depth map

            //printf("depth @ x %d  y  %d: %d %d %d", l[0], l[1], d.c1, d.c2, d.c3);

            // depth map returns in millimeters we want 10cm increments so /100
            beginActR = sin(cameraAngle) * (p1.val[0]/100);//.c1/100);
            endActR = sin(cameraAngle) * (p2.val[0]/100);//.c1/100);

            obLine.beginY = beginActR * sin(theta1);
            obLine.endY = endActR * sin(endTheta1);

            obLine.beginX = beginActR * cos(theta1);
            obLine.endX = endActR * cos(endTheta1);
#endif

            obstLines->addLine(obLine);
        }

#ifdef TEST

        printf("Writing frame: %d\n", numFrames);
        cv::imwrite(pathName + "test_" + std::to_string(numFrames) + ".jpg" , outMat);
#endif

        numFrames++;
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
//    start.lock();
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
            //GPSMessage* gpsMessage = Processor::decode_gps(message.message(), true);
            //setGPS(gpsMessage);
            //std::cout << "decode_gps" << std::endl;
        } else if (header == headers1.WINTERVAL) {
            Interval* interval = Processor::decode_interval(message.message());
            setInterval(interval->interval);
        } else if (header == headers1.WCLOSE) {
            setRunning(false);
            std::cout << "Supposed to close" << std::endl;
        } else if (header == headers1.WSTART) {
//            start.unlock();
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
