
#include "ZMQCameraStatsPublisher.hpp"

#include <time.h>

#include <iostream>
#include <sstream>
#include <inttypes.h>

using namespace std;

ZMQCameraStatsPublisher::ZMQCameraStatsPublisher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    socket.connect(socketAddressStream.str().c_str());

    uint64_t highWaterMark = 4;
    //socket.setsockopt(ZMQ_SNDHWM, (const void*)&highWaterMark, sizeof(highWaterMark));
}

void ZMQCameraStatsPublisher::processStats(const Spyder3Stats& stats){
    zmq::message_t cameraIDMessage(1);
    memcpy((void*) cameraIDMessage.data(), "C", 1);
    socket.send(cameraIDMessage);

    zmq::message_t timeMessage(sizeof(stats.time));
    memcpy((void*)timeMessage.data(), &stats.time, sizeof(stats.time));
    socket.send(timeMessage, ZMQ_SNDMORE);

    stringstream dataStream;
    dataStream << stats.imageCount << "," << stats.frameRate << "," << stats.framesDropped;

    string dataString = dataStream.str();
    zmq::message_t message(dataString.size());
    memcpy((void*)message.data(), dataString.data(), dataString.size());
    socket.send(message, 0);
}

