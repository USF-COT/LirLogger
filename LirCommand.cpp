/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */

#include "LirCommand.hpp"

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include <syslog.h>
#include <iostream>
#include <string>
#include <map>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

// Taken from http://stackoverflow.com/questions/5612182/convert-string-with-explicit-escape-sequence-into-relative-character 
string unescape(const string& s)
{
    string res;
    string::const_iterator it = s.begin();
    while (it != s.end())
    {
        char c = *it++;
        if (c == '\\' && it != s.end())
        {
            switch (*it++) {
                case '\\': c = '\\'; break;
                case 'r': c = '\r'; break;
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                    // all other escapes
                default:
                    continue;
            }
        }
        res += c;
    }

    return res;
}

#define DEFAULTOUTPUTFOLDER "/data1/LirLoggerData"

LirCommand* LirCommand::m_pInstance = NULL;

LirCommand::LirCommand() : deploymentSet(false), running(false), outputFolder(DEFAULTOUTPUTFOLDER), camera(NULL){
    commands["start"] = &LirCommand::receiveStartCommand;
    commands["stop"] = &LirCommand::receiveStopCommand; 
    commands["status"] = &LirCommand::receiveStatusCommand;

    commands["deployment"] = &LirCommand::receiveSetDeploymentCommand;

    commands["camera"] = &LirCommand::receiveSetCameraCommand;

    commands["clear_sensors"] = &LirCommand::receiveClearSensorsCommand;
    commands["add_sensor"] = &LirCommand::receiveAddSensorCommand;
    commands["set_fields"] = &LirCommand::receiveSetFieldsCommand;
    commands["get_value"] = &LirCommand::receiveGetSensorValue;
    commands["get_value_history"] = &LirCommand::receiveGetSensorValueHistory;
}

LirCommand::~LirCommand(){
}

LirCommand* LirCommand::Instance()
{
    if(!m_pInstance){
        m_pInstance = new LirCommand(); 
    }
    return m_pInstance;
}

string LirCommand::receiveStatusCommand(const string command){
    commandMutex.lock();
    stringstream response;

    // Compose system status
    string status = this->running ? "RUNNING":"STOPPED";
    response << status << "\r\n";

    // Compose output type
    if(deploymentSet){
        response << "Authority:" << this->authority << "\r\n";
        response << "Deployment ID:" << this->deploymentID << "\r\n";
        response << "Cruise:" << this->cruise << "\r\n";
        response << "Station:" << this->station << "\r\n";
        response << "UDR:" << this->UDR << "\r\n";
    } else {
        response << "ERROR:DEPLOYMENT NOT CONFIGURED" << "\r\n";
    }

    // Compose camera status
    if(this->camera){
        response << "Camera:" << this->camera->getMAC() << "," << this->camera->getNumBuffers() << "\r\n";
    } else {
        response << "ERROR:CAMERA NOT CONFIGURED" << "\r\n";
    }

    // Compose sensor statuses
    for(unsigned int i=0; i < sensors.size(); ++i){
        response << "Sensor[" << i << "]" << sensors[i]->getName() << ":time(number),frame_id(number),folder_id(number)";
        vector<FieldDescriptor> fields = sensors[i]->getFieldDescriptors();
        for(unsigned int j=0; j < fields.size(); ++j){
            string type = fields[j].isNum ? "number" : "text";
            response << "," << fields[j].name << "(" << type << ")";
        }
        response << "\r\n";
    }

    string resString = response.str();

    commandMutex.unlock();

    return resString;
}

string LirCommand::receiveStartCommand(const string command){
    if(this->startLogger()){
        return string();
    } else {
        return string("Error starting logger. Check status for errors.");
    }
}

bool LirCommand::startLogger(){
    if(!this->running && this->deploymentSet){
        commandMutex.lock();
        if(this->camera) this->camera->start();
        for(unsigned int i=0; i < sensors.size(); ++i){
            sensors[i]->Connect();
        }
        this->running = true;
        commandMutex.unlock();
        return true;
    } else {
        return false;
    }
}

string LirCommand::receiveStopCommand(const string command){
    this->stopLogger();
    return string();
}

bool LirCommand::stopLogger(){
    if(running){
        commandMutex.lock();
        if(this->camera) this->camera->stop();
        for(unsigned int i=0; i < sensors.size(); ++i){
            sensors[i]->Disconnect();
        }
        this->running = false;
        commandMutex.unlock();
    }
}

string LirCommand::parseCommand(const string command){
    string key = command.substr(0,command.find_first_of(" \n\r"));
    if(commands.count(key) > 0){
        return commands[key](this,command);
    } else {
        stringstream response;
        response << "Unrecognized command: " << key <<". Full buffer: " << command << "\r\n";
        return response.str();
    }
}

string LirCommand::receiveSetDeploymentCommand(const string command){
    vector<string> tokens;
    boost::escaped_list_separator<char> sep('\\',' ','\"');
    boost::tokenizer<boost::escaped_list_separator <char> > tok(command,sep);
    BOOST_FOREACH(string t, tok){
        syslog(LOG_DAEMON|LOG_INFO,"Token: %s",t.c_str());
        tokens.push_back(t);
    }

    if(tokens.size() >= 6){
        this->authority = string(tokens[1]);
        try{
            this->deploymentID = boost::lexical_cast<unsigned int>(tokens[2]);
        } catch ( boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR,"Invalid deployment ID passed to Lir Command Parser: %s is not an integer.",tokens[2].c_str());
            return string("Unable to parse deployment ID");
        }
        this->cruise = string(tokens[3]);
        this->station = string(tokens[4]);
        this->UDR = string(tokens[5]);

        this->deploymentSet = true;

        // Pass new folder to listeners
        setListenersOutputFolder();
    } 
    return string();
}

string LirCommand::receiveSetCameraCommand(const string command){
    vector<string> tokens;
    boost::escaped_list_separator<char> sep('\\',' ','\"');
    boost::tokenizer<boost::escaped_list_separator <char> > tok(command,sep);
    BOOST_FOREACH(string t, tok){
        syslog(LOG_DAEMON|LOG_INFO,"Token: %s",t.c_str());
        tokens.push_back(t);
    }

    if(tokens.size() >= 3){
        if(this->running){
            return string("Cannot setup camera while system is running.  Please stop the system before changing any settings.");
        }
        string MAC = string(tokens[1]);
        int numBuffers = 32;
        try{
            numBuffers = boost::lexical_cast<unsigned int>(tokens[2]);
        } catch (boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR, "You must pass an integer for the number of buffers. Using default 32.");
        }
        this->camera = new Spyder3Camera(MAC.c_str(), numBuffers);
        this->connectCameraListeners();
    }
    return string();
}

// Config parsing functions follow
string LirCommand::generateFolderName(){
    if (this->deploymentSet){
        stringstream ss;
        ss << this->outputFolder << "/" << this->authority << "." << this->deploymentID << ".";
        ss << this->cruise << "." << this->station << ".";
        ss << this->UDR;
        return ss.str();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Unable to generate logger path.  Deployment details not set.");
        return string();        
    }
}

void LirCommand::setListenersOutputFolder(){
    string fullPath = this->generateFolderName();
    syslog(LOG_DAEMON|LOG_INFO,"Output path changed to: %s", fullPath.c_str());
    syslog(LOG_DAEMON|LOG_INFO,"Changing tiff writer path.");
    if(this->writer) this->writer->changeFolder(fullPath);
    syslog(LOG_DAEMON|LOG_INFO,"Changing SQLite Writer paths.");
    for(unsigned int i=0; i < sensors.size(); ++i){
        sensorWriters[i]->changeFolder(fullPath);
    }
}

void LirCommand::connectCameraListeners(){
    string fullPath = this->generateFolderName();
    this->writer = new Spyder3TiffWriter(fullPath);
    this->camera->addListener(this->writer);
    this->camStats = new MemoryCameraStatsListener();
    this->camera->addStatsListener(this->camStats);
}

void LirCommand::connectSensorSQLWriters(){
    string fullPath = this->generateFolderName();
    map<unsigned int, EthSensor*>::iterator it;
    for(it = this->sensors.begin(); it != this->sensors.end(); it++){
        EthSensor* sensor = it->second;
        syslog(LOG_DAEMON|LOG_INFO,"Adding SQLite Writer.");
        LirSQLiteWriter* sensorWriter = new LirSQLiteWriter(this->writer, sensor, fullPath);
        this->sensorWriters[sensor->getID()] = sensorWriter;
    }
}

Spyder3Stats LirCommand::getCameraStats(){
    Spyder3Stats retVal;

    commandMutex.lock();
    retVal = this->camStats->getCurrentStats();
    commandMutex.unlock();

    return retVal;
}

string LirCommand::receiveClearSensorsCommand(const string command){
    if(this->running){
        return string("Cannot change logger settings while it is running.  Please stop the system and try again.");
    }

    // Clear sensor lists
    map<unsigned int, EthSensor*>::iterator it;
    for (it=this->sensors.begin(); it != this->sensors.end(); ++it){
        delete it->second;
    }
    sensors.clear();

    map<unsigned int, LirSQLiteWriter*>::iterator wt;
    for (wt=this->sensorWriters.begin(); wt != this->sensorWriters.end(); ++wt){
        delete wt->second;
    }
    sensorWriters.clear();

    map<unsigned int, MemoryEthSensorListener*>::iterator mt;
    for (mt=this->sensorMems.begin(); mt != this->sensorMems.end(); ++mt){
        delete mt->second;
    }
    sensorMems.clear();

    return string();
}

string LirCommand::receiveAddSensorCommand(const string command){
    if(this->running){
        return string("Cannot change logger settings while it is running.  Please stop the system and try again.");
    }

    vector<string> tokens;
    boost::escaped_list_separator<char> sep('\\',' ','\"');
    boost::tokenizer<boost::escaped_list_separator <char> > tok(command,sep);
    BOOST_FOREACH(string t, tok){
        syslog(LOG_DAEMON|LOG_INFO,"Token: %s",t.c_str());
        tokens.push_back(t);
    }

    if(tokens.size() >= 8){
        string authority = string(tokens[1]);
        unsigned int sensorID = 0;
        try{
             sensorID = boost::lexical_cast<unsigned int>(tokens[2]);
        } catch ( boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR,"Invalid sensor ID passed to Lir Command Parser: %s is not an integer.",tokens[2].c_str());
            return string("Unable to parse sensor ID");
        }
        string sensorName = string(tokens[3]);
        string startChars = unescape(string(tokens[4]));
        string endChars = unescape(string(tokens[5]));
        string delimeter = unescape(string(tokens[6]));
        string lineEnd = unescape(string(tokens[7]));

        string serialServer = string(tokens[8]);
        unsigned int port = 4000;
        try{
             port = boost::lexical_cast<unsigned int>(tokens[9]);
        } catch ( boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR,"Invalid serial server port passed to Lir Command Parser: %s is not an integer.",tokens[8].c_str());
            return string("Unable to parse serial server port");
        }

        map <unsigned int, FieldDescriptor> fields;
        for(int i=10; i < tokens.size()-2; i+=4){
            FieldDescriptor d;
            unsigned int fieldID = 0;
            try{
                fieldID  = boost::lexical_cast<unsigned int>(tokens[i]);
            } catch ( boost::bad_lexical_cast const &){
                syslog(LOG_DAEMON|LOG_ERR,"Invalid field ID passed to Lir Command Parser: %s is not an integer.",tokens[i].c_str());
                return string("Unable to parse field ID");
            }
            d.name = string(tokens[i+1]);
            d.units = string(tokens[i+2]);
            d.isNum = boost::iequals(string(tokens[i+3]), "numeric");
            fields[fieldID] = d;
        }
        // Create and start sensor so that clients can read it whenever
        EthSensor* sensor = new EthSensor(sensorID, serialServer, port, sensorName, lineEnd, delimeter, fields, startChars, endChars);
        sensor->Connect();
        this->sensors[sensorID] = sensor;
        MemoryEthSensorListener* memListener = new MemoryEthSensorListener();
        sensors[sensorID]->addListener(memListener);
        sensorMems[sensorID] = memListener;
    }
    return string();
}

/*
vector<EthSensorReadingSet> LirCommand::getSensorSets(){
    vector<EthSensorReadingSet> retVal;

    commandMutex.lock();
    for(unsigned int i=0; i < sensorMems.size(); ++i){
        retVal.push_back(sensorMems[i]->getCurrentReading());
    }
    commandMutex.unlock();

    return retVal;
}
*/

void LirCommand::findLastDeploymentStation(){
    DIR* dp;
    struct dirent *dir;
    struct stat fileStats;

    time_t time = 0;
    time_t maxTime = 0;
    string fileName = string();

    if((dp = opendir(this->outputFolder.c_str())) == NULL){
        syslog(LOG_DAEMON|LOG_ERR,"Unable to open directory %s.",this->outputFolder.c_str());
        return;
    }

    while ((dir = readdir(dp)) != NULL){
        stringstream ss;
        ss << outputFolder << "/" << string(dir->d_name);
        string tempPath = ss.str();
        if(stat(tempPath.c_str(),&fileStats) == 0){
            time = fileStats.st_mtime;
            syslog(LOG_DAEMON|LOG_INFO,"Found file %s with time %d",tempPath.c_str(),time);
            if(time > maxTime){
                maxTime = time;
                fileName = string(dir->d_name);
            }
        }
    }

    vector<string> tokens;
    if(fileName.size() > 0){
        syslog(LOG_DAEMON|LOG_INFO,"Found %s to be the latest directory.",fileName.c_str());
        split(tokens, fileName, boost::is_any_of("."), boost::token_compress_on);
        if(tokens.size() > 1){
            this->authority = string(tokens[0]);
            try{
                this->deploymentID = boost::lexical_cast<unsigned int>(tokens[1]);
            } catch (boost::bad_lexical_cast const &){
                syslog(LOG_DAEMON|LOG_ERR,"Invalid deployment ID passed to Lir Command Parser: %s is not an integer.  Using 0 as default.",tokens[1].c_str());
                this->deploymentID = 0;
            }
            this->cruise = string(tokens[2]);
            this->station = string(tokens[3]);
            this->UDR = string(tokens[4]);
            this->deploymentSet = true;

            syslog(LOG_DAEMON|LOG_ERR,"Set to log to %s",this->generateFolderName().c_str());
        } else {
            syslog(LOG_DAEMON|LOG_ERR,"Unable to parse directory correctly.");
        }
    } else {
        this->authority = "NOAUTH";
        this->deploymentID = 0;
        this->cruise = "NOCRUISE";
        this->station = "NOSTATION";
        this->UDR = "NOUDR";
        this->deploymentSet = true;
        syslog(LOG_DAEMON|LOG_INFO,"No deployment directory found, using name %s",this->generateFolderName().c_str());
    }
}

/*
EthSensor* processSensorNodeProps(DOMNodeList* sensorProps){
    EthSensor* retVal = NULL;
    if(sensorProps){
        map<string,string> params;
        unsigned int port;
        vector<FieldDescriptor> fields;
        for(unsigned int i=0; i < sensorProps->getLength(); ++i){
            DOMNode* node = sensorProps->item(i);
            string key(XMLString::transcode(node->getNodeName()));
            string val(XMLString::transcode(node->getTextContent()));

            // Only element to be handled differently is the fields element
            if(key.compare("Fields") == 0){
                DOMNodeList* fieldsNodes = node->getChildNodes();
                for(unsigned int j=0; j < fieldsNodes->getLength(); ++j){
                    FieldDescriptor d;
                    DOMNode* fieldN = fieldsNodes->item(j);
                    if(XMLString::equals(fieldN->getNodeName(),XMLString::transcode("Field"))){
                        DOMNamedNodeMap* attrs = fieldN->getAttributes();
                        DOMNode* numericA = attrs->getNamedItem(XMLString::transcode("numeric"));
                        DOMNode* nameA = attrs->getNamedItem(XMLString::transcode("name"));
                        d.isNum = XMLString::equals(numericA->getTextContent(),XMLString::transcode("true"));
                        d.name = string(XMLString::transcode(nameA->getTextContent()));
                        syslog(LOG_DAEMON|LOG_INFO,"Found field. Name: %s, Numeric?: %s.",d.name.c_str(),d.isNum?"TRUE":"FALSE");
                        fields.push_back(d);
                    }
                }
            } else {
                params[key] = val;
            }
        }
        try{
            port = boost::lexical_cast<unsigned int>(params["Port"]);
        } catch ( boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR,"Invalid port number in configuration file: %s is not an integer.",params["Port"].c_str());
            return retVal;
        }

        params["StartChars"] = unescape(params["StartChars"]);
        params["EndChars"] = unescape(params["EndChars"]);
        params["LineEnd"] = unescape(params["LineEnd"]);
        params["Delimeter"] = unescape(params["Delimeter"]);

        syslog(LOG_DAEMON|LOG_INFO,"Read %s sensor configuration as - Address: %s:%d, StartChars: %s, EndChars: %s, Delimeter: %s, LineEnd: %s, Fields: %s.",params["Name"].c_str(),params["IPAddress"].c_str(),port,params["StartChars"].c_str(),params["EndChars"].c_str(),params["Delimeter"].c_str(),params["LineEnd"].c_str(),params["Fields"].c_str()); 

        retVal = new EthSensor(params["IPAddress"],port,params["Name"],params["LineEnd"],params["Delimeter"],fields,params["StartChars"],params["EndChars"]);
    }
    return retVal;
}
*/


