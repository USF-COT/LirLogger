
#include <syslog.h>
#include <vector>
#include <iostream>
#include "ZMQEthSensorPublisher.hpp"
#include "ZMQSendUtils.hpp"

ZMQEthSensorPublisher::ZMQEthSensorPublisher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    syslog(LOG_DAEMON|LOG_INFO, "Socket address %s",socketAddressStream.str().c_str());
    socket.connect(socketAddressStream.str().c_str());

    int highWaterMark = 3;
    socket.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));
}

ZMQEthSensorPublisher::~ZMQEthSensorPublisher(){
}

void ZMQEthSensorPublisher::sensorStarting(){
}

void ZMQEthSensorPublisher::processReading(const EthSensorReadingSet& set){
    stringstream datastream;

    if(set.readings.size() > 0)
        datastream << set.readings[0].text;

    vector<EthSensorReading>::const_iterator it;
    for(it = set.readings.begin()+1; it != set.readings.end(); ++it){
        datastream << "," << it->text;
    }

    string dataString = datastream.str();
    sendLirMessage(socket, 'S', set.sensorID, set.time, dataString);
}

void ZMQEthSensorPublisher::sensorStopping(){
}

