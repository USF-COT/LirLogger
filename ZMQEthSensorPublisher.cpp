
#include <syslog.h>
#include <vector>
#include <iostream>
#include "ZMQEthSensorPublisher.hpp"

ZMQEthSensorPublisher::ZMQEthSensorPublisher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    syslog(LOG_DAEMON|LOG_INFO, "Socket address %s",socketAddressStream.str().c_str());
    socket.connect(socketAddressStream.str().c_str());

    //uint64_t highWaterMark = 4;
    //socket.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof highWaterMark);
}

ZMQEthSensorPublisher::~ZMQEthSensorPublisher(){
}

void ZMQEthSensorPublisher::sensorStarting(){
}

void ZMQEthSensorPublisher::processReading(const EthSensorReadingSet& set){
    /*
    stringstream sensorIDStream;
    sensorIDStream << set.sensorID;
    string sensorID = sensorIDStream.str();
    zmq::message_t sensorIDMessage(sensorID.size());
    memcpy((void*)sensorIDMessage.data(), sensorID.data(), sensorID.size());
    socket.send(sensorIDMessage, ZMQ_SNDMORE); 
    */

    zmq::message_t sensorIDMessage(sizeof(set.sensorID));
    memcpy((void*)sensorIDMessage.data(), &set.sensorID, sizeof(set.sensorID));
    socket.send(sensorIDMessage, ZMQ_SNDMORE);

    zmq::message_t timeMessage(sizeof(set.time));
    memcpy((void*)timeMessage.data(), &set.time, sizeof(set.time));
    socket.send(timeMessage, ZMQ_SNDMORE);

    stringstream datastream;

    if(set.readings.size() > 0)
        datastream << set.readings[0].text;

    vector<EthSensorReading>::const_iterator it;
    for(it = set.readings.begin()+1; it != set.readings.end(); ++it){
        datastream << "," << it->text;
    }

    string dataString = datastream.str();

    zmq::message_t message(dataString.size());
    memcpy((void*)message.data(), dataString.data(), dataString.size());
    socket.send(message, 0);
}

void ZMQEthSensorPublisher::sensorStopping(){
}

