
#ifndef ZMQCAMSTATPUBLISHER_HPP
#define ZMQCAMSTATPUBLISHER_HPP

#include <zmq.hpp>
#include "IZMQPublisher.hpp"
#include "ISpyder3StatsListener.hpp"

class ZMQCameraStatsPublisher : public ISpyder3StatsListener, public IZMQPublisher{
private:
    zmq::context_t context;
    zmq::socket_t socket;

public:
    ZMQCameraStatsPublisher();
    ~ZMQCameraStatsPublisher(){};
    virtual void processStats(const Spyder3Stats& stats);
};

#endif
