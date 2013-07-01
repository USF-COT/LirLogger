
#ifndef ZMQIMAGEWRITERSTATSPUSHER_HPP
#define ZMQIMAGEWRITERSTATSPUSHER_HPP

#include <string>
#include <zmq.hpp>
#include "IZMQPublisher.hpp"
#include "ISpyder3ImageWriterListener.hpp"

class ZMQImageWriterStatsPusher : public ISpyder3ImageWriterListener, public IZMQPublisher{
private:
    zmq::context_t context;
    zmq::socket_t socket;

public:
    ZMQImageWriterStatsPusher();
    ~ZMQImageWriterStatsPusher(){};
    virtual void processStats(const Spyder3ImageWriterStats& stats);
};

#endif
