
#ifndef SPY3CAM_HPP
#define SPY3CAM_HPP

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include "ISpyder3Listener.hpp"
#include "ISpyder3StatsListener.hpp"

using namespace std;

class Spyder3Camera{

private:
    char* MAC;
    unsigned int pipelineBufferMax;
    vector<ISpyder3Listener*> listeners;
    vector<ISpyder3StatsListener*> statsListeners;
    
    boost::thread* camThread;
    boost::mutex runMutex;
    boost::mutex listenersMutex;
    boost::mutex statsListenerMutex;
    bool isRunning;

public:
    Spyder3Camera(const char* _MAC, unsigned int _pipelineBufferMax=32);
    ~Spyder3Camera();

    string getMAC();
    unsigned int getNumBuffers();
    
    void addListener(ISpyder3Listener *l);
    void addStatsListener(ISpyder3StatsListener *l);
    void clearListeners();

    bool start();
    bool stop();

    void operator() ();
    
};

#endif
