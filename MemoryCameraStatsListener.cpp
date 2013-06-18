
#include "MemoryCameraStatsListener.hpp"

void MemoryCameraStatsListener::processStats(const Spyder3Stats& stats){
    dataMutex.lock();
    currentStats = stats;
    dataMutex.unlock();
}

Spyder3Stats MemoryCameraStatsListener::getCurrentStats(){
    Spyder3Stats retVal;
    dataMutex.lock();
    retVal = currentStats;
    dataMutex.unlock();
    return retVal;
}
