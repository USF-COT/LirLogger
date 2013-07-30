
#include "SerialSensor.hpp"
#include <syslog.h>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>

using namespace std;

SerialSensor::SerialSensor(const unsigned int _sensorID, const string _port, const int _baud, const string _name, const string _lineEnd, const string _delimeter, const vector<FieldDescriptor> _fields, const string _startChars, const string _endChars) : sensorID(_sensorID), port_string(_port), baud(_baud), name(_name), lineEnd(_lineEnd), delimeter(_delimeter), fields(_fields), startChars(_startChars), endChars(_endChars){
    running = false;
}

SerialSensor::~SerialSensor(){
    this->Disconnect(); // Run this in case the sensor is left connected
}

void SerialSensor::setRunning(bool value){
    runMutex.lock();
    this->running = value;
    runMutex.unlock();
}

bool SerialSensor::isRunning(){
    bool retVal;
    runMutex.lock();
    retVal = running;
    runMutex.unlock();

    return retVal;
}

bool SerialSensor::Connect(){
    if(!this->isRunning()){
        syslog(LOG_DAEMON|LOG_INFO,"Connecting %s sensor @ %s",name.c_str(),port_string.c_str());

        try{
            serial = new boost::asio::serial_port(ios);
            serial->open(port_string.c_str());
            serial->set_option(boost::asio::serial_port_base::baud_rate(9600));
            serial->set_option(boost::asio::serial_port_base::character_size(8));
            if(startChars.length() > 0) serial->write_some(boost::asio::buffer(startChars));
            boost::asio::async_read_until(*serial,buf,lineEnd,boost::bind(&SerialSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            readThread = new boost::thread(boost::ref(*this));
            this->setRunning(true);
            syslog(LOG_DAEMON|LOG_INFO,"%s Sensor Connected", name.c_str());
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to connect to ethernet sensor %s @ %s.  Error: %s",name.c_str(),port_string.c_str(),e.what());
        }
    } else {
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s already running",name.c_str());
    }
}

void SerialSensor::parseLine(const boost::system::error_code& ec, size_t bytes_transferred){
    SensorReadingSet set;
    set.time = time(NULL);
    set.sensorID = this->sensorID;
    set.sensorName = this->name;
    if(!ec){
        runMutex.lock();
        if(running){
            // Parse Line Here
            istream is(&buf);
            string line;
            getline(is,line);

            boost::algorithm::trim(line);

            unsigned int i=0;
            boost::char_separator<char> sep(delimeter.c_str(), "", boost::drop_empty_tokens);
            boost::tokenizer< boost::char_separator<char> > tokens(line, sep);
            BOOST_FOREACH(string t, tokens){
                SensorReading r;
                syslog(LOG_DAEMON|LOG_INFO, "Reading @ column %d for field ID %d: %s", i, fields[i].id, t.c_str());
                boost::algorithm::trim(t);
                r.fieldID = fields[i].id;
                r.field = fields[i].name;
                r.text = t;
                if(fields[i].isNum){
                    try{
                        r.num = boost::lexical_cast<double>(t);
                        r.isNum = true;
                    } catch( boost::bad_lexical_cast const &){
                        r.isNum = false;
                    }
                }
                set.readings.push_back(r);
                if (++i == fields.size())
                    break;
            }

            // Pass Reading to Handlers            
            listenersMutex.lock();
            for(i=0; i < listeners.size(); ++i){
                listeners[i]->processReading(set);
            }
            listenersMutex.unlock();

            // Schedule next request
            boost::asio::async_read_until(*serial,buf,lineEnd,boost::bind(&SerialSensor::parseLine,this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); 
        } else {
            ios.stop();
        }
        runMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Error communicating with %s serial server.",name.c_str());
    }
}

bool SerialSensor::Disconnect(){
    if(this->isRunning()){
        this->setRunning(false);

        try{
            if(endChars.length() > 0) serial->write_some(boost::asio::buffer(endChars));
            serial->close();
            delete serial;
        } catch (std::exception& e){
            syslog(LOG_DAEMON|LOG_ERR,"Unable to disconnect ethernet sensor %s @ %s.  Error: %s",name.c_str(),port_string.c_str());
        }
        if(readThread) readThread->join();
        syslog(LOG_DAEMON|LOG_INFO,"Sensor %s disconnected",name.c_str());            
    }
}

void SerialSensor::addListener(ISensorListener *l){
    listenersMutex.lock();
    listeners.push_back(l);
    listenersMutex.unlock();
}

void SerialSensor::clearListeners(){
    listenersMutex.lock();
    BOOST_FOREACH(ISensorListener* l, listeners){
        l->sensorStopping();
    }
    listeners.clear();
    listenersMutex.unlock();
}

void SerialSensor::operator() (){
    listenersMutex.lock();
    BOOST_FOREACH(ISensorListener* l, listeners){
        l->sensorStarting();
    }
    listenersMutex.unlock();

    ios.reset();
    ios.run();
    
    listenersMutex.lock();
    BOOST_FOREACH(ISensorListener* l, listeners){
        l->sensorStopping();
    }
    listenersMutex.unlock(); 
}

unsigned int SerialSensor::getID(){
    return this->sensorID;
}

string SerialSensor::getName(){
    string retVal(this->name);
    return retVal;
}

vector<FieldDescriptor> SerialSensor::getFieldDescriptors(){
    vector<FieldDescriptor> retVal(this->fields);
    return retVal;
}
