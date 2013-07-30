
#include <syslog.h>
#include <vector>
#include <sstream>
#include "ZMQSensorPublisher.hpp"
#include "ZMQSendUtils.hpp"

ZMQSensorPublisher::ZMQSensorPublisher():context(1), socket(context, ZMQ_PUSH){
    stringstream socketAddressStream;
    socketAddressStream << "tcp://localhost:" << PUSH_PORT;
    syslog(LOG_DAEMON|LOG_INFO, "Socket address %s",socketAddressStream.str().c_str());
    socket.connect(socketAddressStream.str().c_str());

    int highWaterMark = 3;
    socket.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));
}

ZMQSensorPublisher::~ZMQSensorPublisher(){
}

void ZMQSensorPublisher::sensorStarting(){
}

void ZMQSensorPublisher::processReading(const SensorReadingSet& set){
    if(set.readings.size() > 0){
        stringstream datastream;
        datastream << set.readings[0].text;

        vector<SensorReading>::const_iterator it;
        for(it = set.readings.begin()+1; it != set.readings.end(); ++it){
            datastream << "," << it->text;
        }
        string dataString = datastream.str();
        sendLirMessage(socket, 'S', set.sensorID, set.time, dataString);
    }
}

void ZMQSensorPublisher::sensorStopping(){
}

