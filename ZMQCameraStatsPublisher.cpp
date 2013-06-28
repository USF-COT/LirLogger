
#include "ZMQCameraStatsPublisher.hpp"
#include "ZMQSendUtils.hpp"

#include <time.h>

#include <iostream>
#include <sstream>
#include <inttypes.h>

using namespace std;

ZMQCameraStatsPublisher::ZMQCameraStatsPublisher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    socket.connect(socketAddressStream.str().c_str());

    int highWaterMark = 4;
    socket.setsockopt(ZMQ_SNDHWM, (const void*)&highWaterMark, sizeof(highWaterMark));
}

void ZMQCameraStatsPublisher::processStats(const Spyder3Stats& stats){
    stringstream dataStream;
    dataStream << stats.imageCount << "," << stats.frameRate << "," << stats.framesDropped;

    string dataString = dataStream.str();
    sendLirMessage(socket, 'C', stats.cameraID, stats.time, dataString);
}

