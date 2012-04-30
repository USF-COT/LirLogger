
#ifndef SPY3CAM_HPP
#define SPY3CAM_HPP

#include <vector>
#include <boost/thread.hpp>
#include "ISpyder3Listener.hpp"

class Spyder3Camera{

private:
    char* MAC;
    unsigned int pipelineBufferMax;
    std::vector<ISpyder3Listener *> listeners;
    
    boost::thread* camThread;
    boost::mutex runMutex;
    bool isRunning;

public:
    Spyder3Camera(const char* _MAC, unsigned int _pipelineBufferMax=32);
    ~Spyder3Camera();
    
    void addListener(ISpyder3Listener *l);
    bool start();
    bool stop();

    void operator() ();
    
};

#endif
