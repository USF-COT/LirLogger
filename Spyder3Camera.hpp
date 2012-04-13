
#ifndef SPY3CAM_HPP
#define SPY3CAM_HPP

#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

class Spyder3Camera{    
    std::string MAC;
    unsigned int pipelineBufferMax;

public:
    Spyder3Camera() : MAC("00-00-00-00-00-00"), pipelineBufferMax(32){}
    Spyder3Camera(const std::string _MAC) : MAC(_MAC), pipelineBufferMax(32){}
    Spyder3Camera(const std::string _MAC, unsigned int _pipelineBufferMax) : MAC(_MAC), pipelineBufferMax(_pipelineBufferMax){}
    ~Spyder3Camera();
    bool Connect();
    bool Disconnect();
};

#endif
