
#include <syslog.h>
#include <vector>
#include <iostream>
#include "ZMQEthSensorPublisher.hpp"

ZMQEthSensorPublisher::ZMQEthSensorPublisher(unsigned int _port):context(1), socket(context, ZMQ_PUB), port(_port){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://*:" << port;
    syslog(LOG_DAEMON|LOG_INFO, "Socket address %s",socketAddressStream.str().c_str());
    socket.bind(socketAddressStream.str().c_str());

    //uint64_t highWaterMark = 4;
    //socket.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof highWaterMark);
}

ZMQEthSensorPublisher::~ZMQEthSensorPublisher(){
}

unsigned int ZMQEthSensorPublisher::getStreamPort(){
    return this->port;
}

void ZMQEthSensorPublisher::sensorStarting(){
}

void ZMQEthSensorPublisher::processReading(const EthSensorReadingSet& set){
    stringstream datastream;
    datastream << set.time;

    vector<EthSensorReading>::const_iterator it;
    for(it = set.readings.begin(); it != set.readings.end(); ++it){
        datastream << ',' << it->text;
    }

    string dataString = datastream.str();

    zmq::message_t message(dataString.size());
    memcpy((void*)message.data(), dataString.data(), dataString.size());
    socket.send(message);
}

void ZMQEthSensorPublisher::sensorStopping(){
}

