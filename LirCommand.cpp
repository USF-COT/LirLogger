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
    pthread_mutex_init(&commandMutex,NULL);
}

LirCommand::~LirCommand(){
    pthread_mutex_destroy(&commandMutex);
    if(outputFolder) free(outputFolder);
}

LirCommand* LirCommand::Instance()
{
    if(!m_pInstance){
        m_pInstance = new LirCommand(); 
    }
    return m_pInstance;
}

bool LirCommand::loadConfig(char* configPath){
    bool parsed = true;

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
    
    return parsed;
}

void LirCommand::setCamera(Spyder3Camera* camera){
    this->camera = camera;
}

void LirCommand::addEthSensor(EthSensor* sensor){
    this->sensors.insert(sensors.end(),sensor);
}

