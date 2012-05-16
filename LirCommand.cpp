/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */

#include "LirCommand.hpp"

#include <syslog.h>
#include <iostream>
#include <string>
#include <map>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

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
    commands["start"] = &LirCommand::receiveStartCommand;
    commands["stop"] = &LirCommand::receiveStopCommand; 
    commands["status"] = &LirCommand::receiveStatusCommand;
}

LirCommand::~LirCommand(){
    if(outputFolder) free(outputFolder);
}

LirCommand* LirCommand::Instance()
{
    if(!m_pInstance){
        m_pInstance = new LirCommand(); 
    }
    return m_pInstance;
}

void LirCommand::receiveStatusCommand(int connection, char* buffer){
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
    send(connection, resString.c_str(), resString.length(), 0);
    
    commandMutex.unlock();
}

void LirCommand::receiveStartCommand(int connection, char* buffer){
    this->startLogger();
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

void LirCommand::receiveStopCommand(int connection, char* buffer){
    this->stopLogger();
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

void LirCommand::parseCommand(int connection, char* buffer){
    string bufString = string(buffer);
    string key = bufString.substr(0,bufString.find_first_of(" \n\r"));
    if(commands.count(key) > 0){
        commands[key](this,connection,buffer);
    } else {
        syslog(LOG_DAEMON|LOG_ERR,"Unrecognized command %s. Buffer %s",key.c_str(),bufString.c_str());
    }
}


// Config parsing functions follow

void LirCommand::ConnectListeners(){
    this->writer = new Spyder3TiffWriter(this->outputFolder);
    this->camera->addListener(this->writer);
    for(unsigned int i=0; i < sensors.size(); ++i){
        LirSQLiteWriter* sensorWriter = new LirSQLiteWriter(this->writer, sensors[i], string(this->outputFolder));
        sensorWriters.push_back(sensorWriter);
    }
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
                    this->outputFolder = (char*)malloc(sizeof(char)*(strlen(text)+1));
                    strncpy(this->outputFolder,text,strlen(text)+1);
                    syslog(LOG_DAEMON|LOG_INFO,"Output Folder is: %s",this->outputFolder);
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

        commandMutex.unlock();
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Cannot reconfigure a running command instance.");
    }

    this->ConnectListeners();

    return parsed;
}

