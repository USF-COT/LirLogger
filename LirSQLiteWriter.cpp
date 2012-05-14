
#include "LirSQLiteWriter.hpp"

#include <utility>
#include <syslog.h>

LirSQLiteWriter::LirSQLiteWriter(Spyder3TiffWriter* _camWriter, EthSensor* _sensor, string outputDirectory) : camWriter(_camWriter), sensor(_sensor){
    dbPath = outputDirectory + "/data.db";
    
    // Make CREATE DATABASE IF NOT EXISTS statement for specified sensor

    // Create a table based on this sensor's field descriptors
    sqlite3_open(dbPath.c_str(), &db);
    

    sqlite3_close(db);

    sensor->addListener(this);
}

LirSQLiteWriter::~LirSQLiteWriter(){
    if(db) sqlite3_close(db);
}

void LirSQLiteWriter::processReading(time_t time, const vector<EthSensorReading>& readings){
    // Must do this here so that the database is opened in the same thread that it is written to
    if(!db){
        syslog(LOG_DAEMON|LOG_INFO, "Opening sqlite3 database to %s",dbPath.c_str());
        sqlite3_open(dbPath.c_str(),&db); 
    }

    pair<unsigned long,unsigned long> frameIDs = this->camWriter->getFolderFrameIDs();
    syslog(LOG_DAEMON|LOG_INFO,"Time:%d,FolderID:%d, FrameID:%d, %d Readings.",time,frameIDs.first,frameIDs.second,readings.size());

    /*
    for(unsigned int i=0; i < readings.size(); ++i){
    }
    */
}
