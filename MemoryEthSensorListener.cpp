
#include <utility>
#include "MemoryEthSensorListener.hpp"

MemoryEthSensorListener::MemoryEthSensorListener(){
    currentSet.time = time(NULL);
    currentSet.sensorName = string("No Reading");
}

MemoryEthSensorListener::~MemoryEthSensorListener(){
}

void MemoryEthSensorListener::sensorStarting(){
}

void MemoryEthSensorListener::processReading(const EthSensorReadingSet& set){
    dataMutex.lock();
    currentSet = set;
    dataMutex.unlock();
}

void MemoryEthSensorListener::sensorStopping(){
}

EthSensorReadingSet MemoryEthSensorListener::getCurrentReading(){
    EthSensorReadingSet retVal;

    dataMutex.lock();
    retVal = currentSet; 
    dataMutex.unlock();
    
    return retVal;
}
