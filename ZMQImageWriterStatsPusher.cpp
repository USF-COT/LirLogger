
#include "ZMQImageWriterStatsPusher.hpp"
#include "ZMQSendUtils.hpp"

#include <iostream>
#include <sstream>

using namespace std;

ZMQImageWriterStatsPusher::ZMQImageWriterStatsPusher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    socket.connect(socketAddressStream.str().c_str());

    int highWaterMark = 4;
    socket.setsockopt(ZMQ_SNDHWM, (const void*)&highWaterMark, sizeof(highWaterMark));
}

void ZMQImageWriterStatsPusher::processStats(const Spyder3ImageWriterStats& stats){
    stringstream dataStream;
    dataStream << stats.folderID << "," << stats.frameID << "," << stats.numBytes;

    string dataString = dataStream.str();
    sendLirMessage(socket, 'F', stats.cameraID, stats.time, dataString); 
}
