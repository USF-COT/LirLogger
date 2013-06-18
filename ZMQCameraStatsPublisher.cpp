
#include "ZMQCameraStatsPublisher.hpp"

#include <time.h>

#include <iostream>
#include <sstream>
#include <inttypes.h>

using namespace std;

ZMQCameraStatsPublisher::ZMQCameraStatsPublisher(unsigned int _port):context(1), socket(context, ZMQ_PUB), port(_port){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://*:" << port;
    socket.bind(socketAddressStream.str().c_str());

    uint64_t highWaterMark = 4;
    //socket.setsockopt(ZMQ_SNDHWM, (const void*)&highWaterMark, sizeof(highWaterMark));
}

void ZMQCameraStatsPublisher::processStats(const Spyder3Stats& stats){
    stringstream dataStream;

    dataStream << time(NULL) << "," << stats.imageCount << "," << stats.frameRate << "," << stats.framesDropped;

    string dataString = dataStream.str();
    zmq::message_t message(dataString.size());
    memcpy((void*)message.data(), dataString.data(), dataString.size());
    socket.send(message);
}

unsigned int ZMQCameraStatsPublisher::getStreamPort(){
    return port;
}

