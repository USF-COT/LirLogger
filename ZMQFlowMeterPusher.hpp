
#ifndef ZMQFLOWMETERPUSHER_HPP
#define ZMQFLOWMETERPUSHER_HPP

#include <zmq.hpp>

#include "FlowMeter.hpp"
#include "IFlowMeterListener.hpp"
#include "IZMQPublisher.hpp"

using namespace std;

class ZMQFlowMeterPusher : public IZMQPublisher, public IFlowMeterListener{
    private:
        zmq::context_t context;
        zmq::socket_t socket;
    
    public:
        ZMQFlowMeterPusher();
        ~ZMQFlowMeterPusher();

        // IFlowMeterListener Methods
        virtual void sensorStarting();
        virtual void processFlowReading(time_t timestamp, int counts, double flowRate);
        virtual void sensorStopping();
};

#endif
