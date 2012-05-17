
#ifndef MEMCAMSTATLISTENER_HPP
#define MEMCAMSTATLISTENER_HPP

#include <boost/thread.hpp>
#include "ISpyder3StatsListener.hpp"

class MemoryCameraStatsListener : public ISpyder3StatsListener{

private:
    boost::mutex dataMutex;
    Spyder3Stats currentStats;

public:
    MemoryCameraStatsListener(){};
    virtual void processStats(const Spyder3Stats* stats);
    Spyder3Stats getCurrentStats();
};

#endif
