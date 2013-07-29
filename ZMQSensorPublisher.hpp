
#ifndef ZMQSENSORPUBLISHER_HPP
#define ZMQSENSORPUBLISHER_HPP

#include <zmq.hpp>

#include "ISensor.hpp"
#include "ISensorListener.hpp"
#include "IZMQPublisher.hpp"

using namespace std;

class ZMQSensorPublisher : public IZMQPublisher, public ISensorListener{
    private:
        zmq::context_t context;
        zmq::socket_t socket;
    
    public:
        ZMQSensorPublisher();
        ~ZMQSensorPublisher();

        // ISensorListener Methods
        virtual void sensorStarting();
        virtual void processReading(const SensorReadingSet& set);
        virtual void sensorStopping();
};

#endif
