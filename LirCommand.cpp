/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */

#include "LirCommand.hpp"
#include "ConfigREST.hpp"
#include "ZMQCameraStatsPublisher.hpp"
#include "ZMQEthSensorPublisher.hpp"

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include <syslog.h>
#include <iostream>
#include <fstream>
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
    this->findLastDeploymentStation();
    commands["start"] = &LirCommand::receiveStartCommand;
    commands["stop"] = &LirCommand::receiveStopCommand; 
    commands["status"] = &LirCommand::receiveStatusCommand;

    commands["deployment"] = &LirCommand::receiveSetDeploymentCommand;
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
    map<unsigned int, EthSensor*>::iterator it;
    for (it=this->sensors.begin(); it != this->sensors.end(); ++it){
        unsigned int sensorID = it->first;
        EthSensor* sensor = it->second;
        response << "Sensor[" << sensorID << "]:" << sensor->getName() << ":time(number),frame_id(number),folder_id(number)";
        vector<FieldDescriptor> fields = sensor->getFieldDescriptors();
        for(unsigned int j=0; j < fields.size(); ++j){
            string type = fields[j].isNum ? "number" : "text";
            response << "," << fields[j].name << "(ID:" << fields[j].id << " Type:" << type << ")";
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
        map<unsigned int, LirSQLiteWriter*>::iterator wt;
        for (wt=this->sensorWriters.begin(); wt != this->sensorWriters.end(); ++wt){
            wt->second->startLogging();
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
        map<unsigned int, LirSQLiteWriter*>::iterator wt;
        for (wt=this->sensorWriters.begin(); wt != this->sensorWriters.end(); ++wt){
            wt->second->stopLogging();
        }
        this->running = false;
        commandMutex.unlock();
    }
}

string LirCommand::parseCommand(const string command){
    string key = command.substr(0,command.find_first_of(" \n\r"));
    if(commands.count(key) > 0){
        syslog(LOG_DAEMON|LOG_INFO, "Received recognized command: %s", command.c_str());
        return commands[key](this,command);
    } else {
        stringstream response;
        response << "Unrecognized command: " << key <<". Full buffer: " << command << "\r\n";
        return response.str();
    }
}

string LirCommand::receiveSetDeploymentCommand(const string command){
    unsigned int sentDeploymentID;
    vector<string> tokens;
    boost::escaped_list_separator<char> sep('\\',' ','\"');
    boost::tokenizer<boost::escaped_list_separator <char> > tok(command,sep);
    BOOST_FOREACH(string t, tok){
        syslog(LOG_DAEMON|LOG_INFO,"Token: %s",t.c_str());
        tokens.push_back(t);
    }

    if(tokens.size() == 5){
        this->authority = string(tokens[1]);
        this->authorityIP = string(tokens[2]);
        try{
            sentDeploymentID = boost::lexical_cast<unsigned int>(tokens[3]);
        } catch ( boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR,"Invalid deployment ID passed to Lir Command Parser: %s is not an integer.",tokens[2].c_str());
            return string("Unable to parse deployment ID");
        }
        this->UDR = string(tokens[4]);
        
        if (this->deploymentID != sentDeploymentID){
            this->deploymentID = sentDeploymentID;
            Json::Value config;
            if(loadConfiguration(&config, this->authorityIP, this->deploymentID)){
                this->setupUDR(config);
                //setListenersOutputFolder();
                syslog(LOG_DAEMON|LOG_INFO, "%s set deployment ID to %d and UDR name to %s", this->authority.c_str(), this->deploymentID, this->UDR.c_str());
            } else {
                syslog(LOG_DAEMON|LOG_ERR, "Error loading configuration");
            }
        } else {
            syslog(LOG_DAEMON|LOG_INFO, "Already setup for deployment %d", this->deploymentID);
        }
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Invalid number of arguments sent to receiveSetDeployment.  Expecting 3, received %d", tokens.size()-1);
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

void LirCommand::addSensor(const Json::Value& logger, const Json::Value& sensorConfig){
    unsigned int sensorID = sensorConfig["id"].asUInt();
    string sensorName = sensorConfig["name"].asString();
    string startChars = unescape(sensorConfig["start_chars"].asString());
    string endChars = unescape(sensorConfig["end_chars"].asString());
    string delimeter = unescape(sensorConfig["delimeter"].asString());
    string lineEnd = unescape(sensorConfig["line_end"].asString());

    string serialServer = logger["serial_server_host"].asString();
    unsigned int port = sensorConfig["serial_server_port"].asUInt();

    vector <FieldDescriptor> fields;
    const Json::Value fieldsJSON = sensorConfig["fields"];
    for(unsigned int i=0; i < fieldsJSON.size(); i++){
        FieldDescriptor d;
        d.id = fieldsJSON[i]["output_order"].asUInt();
        d.name = fieldsJSON[i]["name"].asString();
        d.units = fieldsJSON[i]["units"].asString();
        d.isNum = fieldsJSON[i]["is_numeric"].asBool();
        fields.push_back(d);
    }

    EthSensor* sensor = new EthSensor(sensorID, serialServer, port, sensorName, lineEnd, delimeter, fields, startChars, endChars);
    sensor->Connect();
    this->sensors[sensorID] = sensor;

    ZMQEthSensorPublisher* zmqPublisher = new ZMQEthSensorPublisher();
    sensor->addListener(zmqPublisher);
    this->sensorPublishers[sensorID] = zmqPublisher;

    string fullPath = this->generateFolderName();
    LirSQLiteWriter* sensorWriter = new LirSQLiteWriter(this->writer, sensor, fullPath);
    this->sensorWriters[sensor->getID()] = sensorWriter;
}

string LirCommand::setupUDR(const Json::Value& response){
    const Json::Value loggers = response["system"]["loggers"];

    if(!loggers.isNull()){
        for(int i = 0; i < loggers.size(); i++){
            if (loggers[i]["name"] == this->UDR){
                // Load configuration here
                syslog(LOG_DAEMON|LOG_INFO, "Found configuration for %s", this->UDR.c_str());
                if(this->running){
                    syslog(LOG_DAEMON|LOG_INFO, "Logger was running for previous deployment.  Stopping old deployment and reconfiguring for new one.");
                    this->stopLogger();
                }

                // Setup Cruise
                this->cruise = response["cruise"]["name"].asString();
                this->station = response["station"]["abbreviation"].asString();

                // Clear sensor lists
                this->sensors.clear();
                this->sensorWriters.clear();
                this->sensorPublishers.clear();

                // Setup Camera
                const Json::Value config = loggers[i]["configuration"];
                const Json::Value camera_config = config["camera"];

                unsigned int cameraID = camera_config["id"].asUInt();
                unsigned int num_buffers = config["camera_num_buffers"].asUInt();
                this->camera = new Spyder3Camera(cameraID, camera_config["MAC"].asString().c_str(), num_buffers);
                string fullPath = this->generateFolderName();
                this->writer = new Spyder3TiffWriter(fullPath);
                this->camera->addListener(this->writer);
                this->camStats = new MemoryCameraStatsListener();
                this->camera->addStatsListener(this->camStats);
                if(this->camStatsPublisher)
                    delete this->camStatsPublisher;
                this->camStatsPublisher = new ZMQCameraStatsPublisher();
                this->camera->addStatsListener(this->camStatsPublisher);

                // Setup Sensors
                const Json::Value sensors = config["sensors"];
                for(int j=0; j < sensors.size(); j++){
                    this->addSensor(loggers[i], sensors[j]);
                }

                // Store JSON Response in Deployment Folder for Next Startup
                string configJSONPath = this->generateFolderName() + "/config.json";
                ofstream configJSONOF;
                configJSONOF.open(configJSONPath.c_str());
                configJSONOF << response;
                configJSONOF.close();
            }
        }
    }
    return "";
}

void LirCommand::setListenersOutputFolder(){
    string fullPath = this->generateFolderName();
    syslog(LOG_DAEMON|LOG_INFO,"Output path changed to: %s", fullPath.c_str());
    syslog(LOG_DAEMON|LOG_INFO,"Changing tiff writer path.");
    if(this->writer) this->writer->changeFolder(fullPath);
    syslog(LOG_DAEMON|LOG_INFO,"Changing SQLite Writer paths.");
    map<unsigned int, LirSQLiteWriter*>::iterator wt;
    for (wt=this->sensorWriters.begin(); wt != this->sensorWriters.end(); ++wt){
        wt->second->changeFolder(fullPath);
    }
}

Spyder3Stats LirCommand::getCameraStats(){
    Spyder3Stats retVal;

    commandMutex.lock();
    retVal = this->camStats->getCurrentStats();
    commandMutex.unlock();

    return retVal;
}

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
            if(dir->d_name[0] == '.'){
                syslog(LOG_DAEMON|LOG_INFO,"Ignoring hidden directory");
                continue;
            } else if (time > maxTime){
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

            string configPath = this->generateFolderName() + "/config.json";
            ifstream configStream;
            configStream.open(configPath.c_str());
            Json::Value root;
            Json::Reader reader;

            if(reader.parse(configStream, root)){
                this->setupUDR(root);
            }
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

