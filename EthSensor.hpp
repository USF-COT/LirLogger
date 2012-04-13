#include <string>
#include <vector>

class EthSensor{    
    std::string name;
    std::string lineEnd;
    std::string delimeter;
    std::vector<std::string> fields;
    std::string startChars;
    std::string endChars;

public:
    EthSensor(const std::string _name, const std::string _lineEnd, const std::string _delimeter, const std::vector<std::string> _fields, const std::string _startChars, const std::string _endChars) : name(_name), lineEnd(_lineEnd), delimeter(_delimeter), fields(_fields), startChars(_startChars), endChars(_endChars){}
    ~EthSensor(){}
    bool Connect(){}
    bool Disconnect(){}
};

