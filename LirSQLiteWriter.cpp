
#include "LirSQLiteWriter.hpp"

#include <utility>
#include <syslog.h>
#include <string>
#include <iostream>
#include <sstream>

LirSQLiteWriter::LirSQLiteWriter(Spyder3TiffWriter* _camWriter, EthSensor* _sensor, string outputDirectory) : camWriter(_camWriter), sensor(_sensor){
    char* errMsg;

    dbPath = outputDirectory + "/data.db";
    
    // Make CREATE DATABASE IF NOT EXISTS statement for specified sensor

    // Create a table based on this sensor's field descriptors
    sqlite3_open(dbPath.c_str(), &db);
    
    stringstream ss;
    stringstream is;
    stringstream qMarks;
    ss << "CREATE TABLE IF NOT EXISTS main." << sensor->getName() << " (unix_timestamp INTEGER PRIMARY KEY NOT NULL, folder_id INTEGER NOT NULL, frame_id INTEGER NOT NULL";
    is << "INSERT INTO main." << sensor->getName() << " (unix_timestamp, folder_id, frame_id";
    qMarks << " (?,?,?";
    this->fields = sensor->getFieldDescriptors();
    for(unsigned int i=0; i < fields.size(); ++i){
        string type = fields[i].isNum ? "REAL":"TEXT";
        ss << ", " << fields[i].name << " " << type << " NOT NULL";
        is << ", " << fields[i].name;
        qMarks << ",?";
    }
    ss << ")"; // Close column descriptions 
    is << ")";
    qMarks << ")";

    is << " VALUES " << qMarks.str();

    string query = ss.str();
    if(sqlite3_exec(db,query.c_str(),NULL,NULL,&errMsg) == SQLITE_OK){
        insertStmt = is.str();
        syslog(LOG_DAEMON|LOG_INFO, "Created database for sensor %s.  Query: %s.  Data INSERT stmt: %s",sensor->getName().c_str(),query.c_str(), insertStmt.c_str());
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "SQLite create database failed for sensor %s.  Query: %s. Message: %s.",sensor->getName().c_str(),query.c_str(),errMsg);
    }

    sqlite3_close(db);

    sensor->addListener(this);
}

LirSQLiteWriter::~LirSQLiteWriter(){
}

void LirSQLiteWriter::sensorStarting(){
    syslog(LOG_DAEMON|LOG_INFO, "Opening sqlite3 database to %s",dbPath.c_str());
    sqlite3_open(dbPath.c_str(),&db);
}

void LirSQLiteWriter::processReading(time_t time, const vector<EthSensorReading>& readings){
    if(readings.size() >= fields.size()){
        pair<unsigned long,unsigned long> frameIDs = this->camWriter->getFolderFrameIDs();
        sqlite3_stmt* pStmt = NULL;
        if(sqlite3_prepare(db,insertStmt.c_str(),1024,&pStmt,NULL) == SQLITE_OK){
            // Place time in the beginning
            sqlite3_bind_int64(pStmt,1,(int64_t)time);
            // Place frame information
            sqlite3_bind_int64(pStmt,2,(int64_t)frameIDs.first);
            sqlite3_bind_int64(pStmt,3,(int64_t)frameIDs.second);

            // Fill in the rest of the fields
            for(unsigned int i=0; i < fields.size(); ++i){
                if(fields[i].isNum){ // INSERT double value if number field
                    double val = -1;
                    if(readings[i].isNum){
                       val = readings[i].num; 
                    } else {
                        syslog(LOG_DAEMON|LOG_ERR,"Shift error when parsing reading %s as number for expected field %s.  Storing -1.",readings[i].field.c_str(),fields[i].name.c_str());
                    }
                    sqlite3_bind_double(pStmt,i+4,val);
                } else { // INSERT as text if text field
                    sqlite3_bind_text(pStmt,i+4,readings[i].text.c_str(),readings[i].text.size(),SQLITE_TRANSIENT);
                }
            }
            if(sqlite3_step(pStmt) == SQLITE_ERROR){
                syslog(LOG_DAEMON|LOG_ERR,"Unable to insert reading for %s sensor.  Error: %s.",sensor->getName().c_str(),sqlite3_errmsg(db));
            }
            sqlite3_finalize(pStmt);
        } else {
            syslog(LOG_DAEMON|LOG_ERR, "Unable to create prepared statement to INSERT readings for sensor %s.  Error: %s",sensor->getName().c_str(),sqlite3_errmsg(db));
        }    
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Fewer readings than expected (%d) received (%d) for SQLiteWriter.  Disregarding this reading.", fields.size(), readings.size());
    }
}

void LirSQLiteWriter::sensorStopping(){
    if(db) sqlite3_close(db);
    syslog(LOG_DAEMON|LOG_INFO, "Sqlite3 database @ %s closed.",dbPath.c_str());
}
