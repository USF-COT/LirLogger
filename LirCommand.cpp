/*
 * LirCommand.cpp - Responsible for providing a thread-safe overview command interface to all Lir components
 *
 * By: Michael Lindemuth
 */

#include "LirCommand.hpp"

#include <syslog.h>
#include <iostream>
#include <string>

#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#define SCHEMAPATH "LirConfigSchema.xsd"

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
    commands["go"] = &LirCommand::receiveStartCommand;
    commands["stop"] = &LirCommand::receiveStopCommand; 
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

void LirCommand::receiveStartCommand(int connection, char* buffer){
    this->startLogger();
}

bool LirCommand::startLogger(){
    if(!this->running){
        commandMutex.lock();
        if(this->camera) this->camera->start();
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
        this->running = false;
        commandMutex.unlock();
    }
}

void LirCommand::parseCommand(int connection, char* buffer){
    std::string bufString = std::string(buffer);
    std::string key = bufString.substr(0,bufString.find_first_of(" \n\r"));
    if(commands.count(key) > 0){
        commands[key](this,connection,buffer);
    } else {
        syslog(LOG_DAEMON|LOG_ERR,"Unrecognized command %s. Buffer %s",key.c_str(),bufString.c_str());
    }
}

void LirCommand::ConnectListeners(){
    this->writer = new Spyder3TiffWriter(this->outputFolder);
    this->camera->addListener(this->writer);
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

