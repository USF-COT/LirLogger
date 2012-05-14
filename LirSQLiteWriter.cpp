
#include "LirSQLiteWriter.hpp"

#include <utility>
#include <syslog.h>

LirSQLiteWriter::LirSQLiteWriter(Spyder3TiffWriter* _camWriter, string outputDirectory) : db(NULL), camWriter(_camWriter){
    dbPath = outputDirectory + "/data.db";
}

LirSQLiteWriter::~LirSQLiteWriter(){
    if(db) sqlite3_close(db);
}

void LirSQLiteWriter::processReading(time_t time, const vector<EthSensorReading>& readings){
    if(!db){
        syslog(LOG_DAEMON|LOG_INFO, "Opening sqlite3 database to %s",dbPath.c_str());
        sqlite3_open(dbPath.c_str(),&db); 
        
        // INSERT new table for sensor if necessary
        
    }

    pair<unsigned long,unsigned long> frameIDs = this->camWriter->getFolderFrameIDs();
    syslog(LOG_DAEMON|LOG_INFO,"Time:%d,FolderID:%d, FrameID:%d, %d Readings.",time,frameIDs.first,frameIDs.second,readings.size());

    /*
    for(unsigned int i=0; i < readings.size(); ++i){
    }
    */
}
