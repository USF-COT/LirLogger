
#include "Sensor.hpp"

class NXICCTD : Sensor{
    friend class boost::serialization::access;
    std::string IPAddress;
    unsigned int port;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        // save/load base class information
        ar & boost::serialization::base_object<Sensor>(*this);
        ar & IPAddress;
        ar & port;
    }

public:
    NXICCTD(std::string _IPAddress, unsigned int _port) : IPAddress(_IPAddress), port(_port){}
    ~NXICCTD(){} 
};

