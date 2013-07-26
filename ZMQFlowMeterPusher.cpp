
#include <syslog.h>
#include <vector>
#include <iostream>
#include "ZMQFlowMeterPusher.hpp"
#include "ZMQSendUtils.hpp"

ZMQFlowMeterPusher::ZMQFlowMeterPusher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    syslog(LOG_DAEMON|LOG_INFO, "Socket address %s",socketAddressStream.str().c_str());
    socket.connect(socketAddressStream.str().c_str());

    int highWaterMark = 3;
    socket.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));
}

ZMQFlowMeterPusher::~ZMQFlowMeterPusher(){
}

void ZMQFlowMeterPusher::sensorStarting(){
}

void ZMQFlowMeterPusher::processFlowReading(time_t timestamp, int counts, double flowRate){
    stringstream datastream;

    datastream << counts << "," << flowRate;

    string dataString = datastream.str();
    sendLirMessage(socket, 'W', 1, timestamp, dataString);
}

void ZMQFlowMeterPusher::sensorStopping(){
}

