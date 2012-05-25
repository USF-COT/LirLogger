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

#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#define SCHEMAPATH "LirConfigSchema.xsd"

using namespace std;
using namespace xercesc;

class ConfigXMLErrorHandler: public ErrorHandler
{
    private:
        void logError(char* level, const SAXParseException &exc){
            XMLFileLoc line = exc.getLineNumber();
            XMLFileLoc pos = exc.getColumnNumber();
            char* publicID = XMLString::transcode(exc.getPublicId());
            char* systemID = XMLString::transcode(exc.getSystemId());
            syslog(LOG_DAEMON|LOG_ERR,"%s parsing XML at position %d of line %d.  Public ID: %s, System ID: %s",level,pos,line, publicID, systemID);
        }

    public:
        ConfigXMLErrorHandler(){}
        void warning(const SAXParseException &exc){
            logError("Warning",exc);
        }
        void error(const SAXParseException &exc){
            logError("Error",exc);
        }
        void fatalError(const SAXParseException &exc){
            logError("FATAL ERROR",exc);
        }
        void resetErrors(){
            syslog(LOG_DAEMON|LOG_INFO,"SAX Errors reset.");
        }
};

LirCommand* LirCommand::m_pInstance = NULL;

LirCommand::LirCommand() : running(false),camera(NULL){
    this->deployment = "default";
    this->stationID = 0;
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
    response << "SYSTEM " << status << "\r\n";

    // Compose output type
    response << "All files being written to " << this->outputFolder << "\r\n";

    // Compose camera status
    if(this->camera){
        response << "Camera connected @ " << this->camera->getMAC() << " with " << this->camera->getNumBuffers() << " buffers\r\n";
    } else {
        response << "Camera NOT CONNECTED\r\n";
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
    this->startLogger();
    return string();
}

bool LirCommand::startLogger(){
    if(!this->running){
        commandMutex.lock();
        if(this->camera) this->camera->start();
        for(unsigned int i=0; i < sensors.size(); ++i){
            sensors[i]->Connect();
        }
        this->running = true;
        commandMutex.unlock();
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
        this->ClearListeners();
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

    if(tokens.size() >= 3 && tokens[1].size() > 0 && tokens[2].size() > 0){
        this->deployment = string(tokens[1]);
        this->stationID = 0;
        stringstream ss;
        ss << tokens[2];
        ss >> this->stationID;

        // Pass new folder to listeners
        setListenersOutputFolder();
    } 
    return string();
}

// Config parsing functions follow
void LirCommand::ClearListeners(){
    this->camera->clearListeners();
    delete this->writer;
    delete this->camStats;
    for(unsigned int i=0; i < sensors.size(); ++i){
        sensors[i]->clearListeners();
        delete sensorWriters[i];
        delete sensorMems[i];
    }
}

string LirCommand::generateFolderName(){
    stringstream ss;
    ss << this->outputFolder << "/" << this->deployment << "-" << this->stationID;
    return ss.str();
}

void LirCommand::setListenersOutputFolder(){
    string fullPath = this->generateFolderName();
    if(this->writer) this->writer->changeFolder(fullPath);
    for(unsigned int i=0; i < sensors.size(); ++i){
        sensorWriters[i]->changeFolder(fullPath);
    }
}

void LirCommand::ConnectListeners(){
    string fullPath = this->generateFolderName();
    this->writer = new Spyder3TiffWriter(fullPath);
    this->camera->addListener(this->writer);
    this->camStats = new MemoryCameraStatsListener();
    this->camera->addStatsListener(this->camStats);
    for(unsigned int i=0; i < sensors.size(); ++i){
        LirSQLiteWriter* sensorWriter = new LirSQLiteWriter(this->writer, sensors[i], fullPath);
        sensorWriters.push_back(sensorWriter);

        MemoryEthSensorListener* memListener = new MemoryEthSensorListener();
        sensors[i]->addListener(memListener);
        sensorMems.push_back(memListener);
    }
}

Spyder3Stats LirCommand::getCameraStats(){
    Spyder3Stats retVal;

    commandMutex.lock();
    retVal = this->camStats->getCurrentStats();
    commandMutex.unlock();

    return retVal;
}

vector<EthSensorReadingSet> LirCommand::getSensorSets(){
    vector<EthSensorReadingSet> retVal;

    commandMutex.lock();
    for(unsigned int i=0; i < sensorMems.size(); ++i){
        retVal.push_back(sensorMems[i]->getCurrentReading());
    }
    commandMutex.unlock();

    return retVal;
}

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
                          // invalid escape sequence - skip it. alternatively you can copy it as is, throw an exception...
                          continue;
            }
        }
        res += c;
    }

    return res;
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
            if(time > maxTime){
                maxTime = time;
                fileName = string(dir->d_name);
            }
        }
    }

    vector<string> tokens;
    if(fileName.size() > 0){
        syslog(LOG_DAEMON|LOG_INFO,"Found %s to be the latest directory.",fileName.c_str());
        split(tokens, fileName, boost::is_any_of("-"), boost::token_compress_on);
        if(tokens.size() > 1){
            this->deployment = string(tokens[0]);
            stringstream ss;
            ss << tokens[1];
            ss >> this->stationID;
            this->stationID++; // Move to next station ID
            syslog(LOG_DAEMON|LOG_ERR,"Set to log to %s-%d",this->deployment.c_str(),this->stationID);
        } else {
            syslog(LOG_DAEMON|LOG_ERR,"Unable to parse directory correctly.");
        }
    } else {
        this->deployment = string("default");
        this->stationID = 0;
        syslog(LOG_DAEMON|LOG_INFO,"No deployment directory found, using name %s-%d",this->deployment.c_str(),this->stationID);
    }
}

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

bool LirCommand::loadConfig(char* configPath){
    bool parsed = true;

    if(!this->running){
        try{
            XMLPlatformUtils::Initialize();
        }
        catch (const XMLException& toCatch){
            char* message = XMLString::transcode(toCatch.getMessage());
            syslog(LOG_DAEMON|LOG_ERR,"Error during initialization!: %s\n",message);
            return false;
        }

        XercesDOMParser* parser = new XercesDOMParser();
        parser->setValidationScheme(XercesDOMParser::Val_Auto);
        parser->setDoNamespaces(true);

        ConfigXMLErrorHandler* eh = new ConfigXMLErrorHandler(); 
        parser->setErrorHandler(eh);
        parser->setDoNamespaces(true);
        parser->setDoSchema(true);

        DOMDocument* doc;
        commandMutex.lock();
        try{
            parser->parse(configPath);
            DOMDocument* doc = parser->getDocument();
            if(doc){
                // DOM tree here, start making objects
                // Get Output Folder
                DOMNodeList* outputFolderEl = doc->getElementsByTagNameNS(XMLString::transcode("*"),XMLString::transcode("OutputFolder"));
                if(outputFolderEl && outputFolderEl->getLength() > 0){
                    DOMNode* node = outputFolderEl->item(0);
                    char* text = XMLString::transcode(node->getTextContent());
                    this->outputFolder = string(text);
                    syslog(LOG_DAEMON|LOG_INFO,"Output Folder is: %s",this->outputFolder.c_str());
                    findLastDeploymentStation();
                }
                DOMNodeList* camNode = doc->getElementsByTagNameNS(XMLString::transcode("*"),XMLString::transcode("GigECamera"));
                if(camNode && camNode->getLength() > 0){
                    char* MACAddress = NULL;
                    int bufferLength = 0;
                    DOMNodeList* camProps = camNode->item(0)->getChildNodes();
                    for(int i=0; i < camProps->getLength(); i++){
                        DOMNode* node = camProps->item(i);
                        if(XMLString::equals(node->getNodeName(),XMLString::transcode("MACAddress"))){
                            char* text = XMLString::transcode(node->getTextContent());
                            MACAddress = (char*)malloc(sizeof(char)*(strlen(text)+1));
                            strncpy(MACAddress,text,strlen(text)+1);
                        } else if(XMLString::equals(node->getNodeName(),XMLString::transcode("BufferLength"))){
                            char* text = XMLString::transcode(node->getTextContent());
                            if(strlen(text) > 0){
                                bufferLength = atoi(text);
                            }
                        }
                    }
                    if(MACAddress){
                        if(bufferLength > 0)
                            this->camera = new Spyder3Camera(MACAddress,bufferLength);
                        else
                            this->camera = new Spyder3Camera(MACAddress);
                        syslog(LOG_DAEMON|LOG_INFO, "Camera @ %s Successfully Configured.",MACAddress);
                    } else {
                        syslog(LOG_DAEMON|LOG_ERR, "Unable to read configuration for camera.");
                    }
                }

                DOMNodeList* sensorNodes = doc->getElementsByTagNameNS(XMLString::transcode("*"),XMLString::transcode("EthernetSensor"));
                if(sensorNodes){
                    for(unsigned int i=0; i < sensorNodes->getLength(); ++i){
                        DOMNodeList* sensorProps = sensorNodes->item(i)->getChildNodes();
                        EthSensor* sensor = processSensorNodeProps(sensorProps);
                        if(sensor) sensors.push_back(sensor);
                    }
                }

            } else {
                syslog(LOG_DAEMON|LOG_ERR,"Unable to load DOM tree from document.");
            }
        } catch(const XMLException& toCatch){
            char* message = XMLString::transcode(toCatch.getMessage());
            syslog(LOG_DAEMON|LOG_ERR, "XML Exception %s.",message);
            parsed = false;
        } catch(const DOMException& toCatch){
            char* message = XMLString::transcode(toCatch.getMessage());
            syslog(LOG_DAEMON|LOG_ERR, "XML Exception %s.",message);
            parsed = false;
        } catch(...){
            syslog(LOG_DAEMON|LOG_ERR, "Unexpected exception");
            parsed = false;
        }

        // Housekeeping!
        delete parser;
        delete eh;
        XMLPlatformUtils::Terminate();

        this->ConnectListeners();

        commandMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Cannot reconfigure a running command instance.");
    }
    return parsed;
}

