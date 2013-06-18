
#ifndef MEMETHSENSORLISTENER_HPP
#define MEMETHSENSORLISTENER_HPP

#include <utility>
#include <vector>
#include <boost/thread.hpp>

#include "EthSensor.hpp"
#include "IEthSensorListener.hpp"

class MemoryEthSensorListener : public IEthSensorListener{
    private:
        boost::mutex dataMutex;
        EthSensorReadingSet currentSet;
    
    public:
        MemoryEthSensorListener();
        ~MemoryEthSensorListener();
        virtual void sensorStarting();
        virtual void processReading(const EthSensorReadingSet& set);
        virtual void sensorStopping();

        EthSensorReadingSet getCurrentReading();
};

#endif
