
#ifndef ZMQETHSENSORPUBLISHER_HPP
#define ZMQETHSENSORPUBLISHER_HPP

#include <zmq.hpp>

#include "EthSensor.hpp"
#include "IEthSensorListener.hpp"
#include "IZMQPublisher.hpp"

using namespace std;

class ZMQEthSensorPublisher : public IZMQPublisher, public IEthSensorListener{
    private:
        zmq::context_t context;
        zmq::socket_t socket;
    
    public:
        ZMQEthSensorPublisher();
        ~ZMQEthSensorPublisher();

        // IEthSensorListener Methods
        virtual void sensorStarting();
        virtual void processReading(const EthSensorReadingSet& set);
        virtual void sensorStopping();
};

#endif
