
#include "LirSQLiteWriter.hpp"

#include <utility>
#include <syslog.h>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>

void LirSQLiteWriter::initDatabase(string outputFolder){
    char* errMsg;
    pathMutex.lock();    
    if(db) sqlite3_close(db);

    dbPath = outputFolder + "/data.db";
    syslog(LOG_DAEMON|LOG_INFO,"SQLite output path: %s",dbPath.c_str());

    // Create a table based on this sensor's field descriptors
    sqlite3_open(dbPath.c_str(), &db);

    stringstream ss;
    stringstream is;
    stringstream qMarks;
    ss << "CREATE TABLE IF NOT EXISTS main.`" << sensor->getName() << "-" << sensor->getID() << "` (unix_timestamp INTEGER PRIMARY KEY NOT NULL, folder_id INTEGER NOT NULL, frame_id INTEGER NOT NULL";
    is << "INSERT INTO main.`" << sensor->getName() << "-" << sensor->getID() << "` (unix_timestamp, folder_id, frame_id";
    qMarks << " (?,?,?";
    this->fields = sensor->getFieldDescriptors();
    vector <FieldDescriptor>::iterator it;
    for(it = fields.begin(); it != fields.end(); ++it){
        FieldDescriptor field = *it;
        if (boost::iequals(field.name, "ignore"))
            continue; // Skip IGNORE fields
        string type = field.isNum ? "REAL":"TEXT";
        ss << ", `" << field.name << "-" << field.id << "` " << type << " NOT NULL";
        is << ", `" << field.name << "-" << field.id << "`";
        qMarks << ",?";
    }
    ss << ")"; // Close column descriptions
    is << ")";
    qMarks << ")";

    is << " VALUES " << qMarks.str();

    string query = ss.str();
    insertStmt = is.str();
    syslog(LOG_DAEMON|LOG_INFO, "Creating database for sensor %s.  Query: %s.  Data INSERT stmt: %s",sensor->getName().c_str(),query.c_str(), insertStmt.c_str());

    if(sqlite3_exec(db,query.c_str(),NULL,NULL,&errMsg) == SQLITE_OK){
        syslog(LOG_DAEMON|LOG_INFO, "Database created successfully");
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "SQLite create database failed for sensor %s.  Query: %s. Message: %s.",sensor->getName().c_str(),query.c_str(),errMsg);
    }

    sqlite3_close(db);
    db = NULL;
    pathMutex.unlock();
}

LirSQLiteWriter::LirSQLiteWriter(Spyder3ImageWriter* _camWriter, ISensor* _sensor, string outputDirectory) : camWriter(_camWriter), sensor(_sensor), db(NULL), logging(false){
    initDatabase(outputDirectory);
    sensor->addListener(this);
    this->lastRowTimeLogged = time(NULL);
}

LirSQLiteWriter::~LirSQLiteWriter(){
    this->stopLogging();
}

void LirSQLiteWriter::sensorStarting(){
}

void LirSQLiteWriter::startLogging(){
    if(!this->logging){
        syslog(LOG_DAEMON|LOG_INFO, "Opening sqlite3 database to %s",dbPath.c_str());
        pathMutex.lock();
        sqlite3_open(dbPath.c_str(),&db);
        this->logging = true;
        pathMutex.unlock();
    }
}

void LirSQLiteWriter::changeFolder(string outputFolder){
    initDatabase(outputFolder);
}

void LirSQLiteWriter::processReading(const SensorReadingSet& set){
    if(!this->logging || set.time == this->lastRowTimeLogged)
        return; // Do nothing if not logging or if this is too high of a resolution

    pathMutex.lock();
    if(db == NULL) // If the database is not open, try to open it again
        sqlite3_open(dbPath.c_str(),&db);

    if(set.readings.size() >= fields.size()){
        unsigned long folderID = this->camWriter->getFolderID();
        unsigned long frameID = this->camWriter->getFrameID();
        sqlite3_stmt* pStmt = NULL;
        if(sqlite3_prepare(db,insertStmt.c_str(),1024,&pStmt,NULL) == SQLITE_OK){
            // Place time in the beginning
            sqlite3_bind_int64(pStmt,1,(int64_t)set.time);
            // Place frame information
            sqlite3_bind_int64(pStmt,2,(int64_t)folderID);
            sqlite3_bind_int64(pStmt,3,(int64_t)frameID);

            // Fill in the rest of the fields
            unsigned int insertIndex = 0; // Need to keep this seperate due to possiblity of ignore columns
            for(unsigned int i=0; i < set.readings.size(); ++i){
                if (boost::iequals(fields[i].name, "ignore"))
                    continue;

                SensorReading reading = set.readings[i];
                if(fields[i].isNum){ // INSERT double value if number field
                    double val = -1;
                    if(reading.isNum){
                       val = reading.num; 
                    } else {
                        syslog(LOG_DAEMON|LOG_ERR,"Shift error when parsing reading %s from sensor with ID %d as number for expected field %s.  Storing -1.",reading.field.c_str(), set.sensorID, fields[i].name.c_str());
                    }
                    sqlite3_bind_double(pStmt,insertIndex+4,val);
                } else { // INSERT as text if text field
                    sqlite3_bind_text(pStmt,insertIndex+4,reading.text.c_str(),reading.text.size(),SQLITE_TRANSIENT);
                }
                insertIndex++;
            }
            if(sqlite3_step(pStmt) == SQLITE_ERROR){
                syslog(LOG_DAEMON|LOG_ERR,"Unable to insert reading for %s sensor.  Error: %s.",set.sensorName.c_str(),sqlite3_errmsg(db));
            }
            sqlite3_finalize(pStmt);
            this->lastRowTimeLogged = set.time;
        } else {
            syslog(LOG_DAEMON|LOG_ERR, "Unable to create prepared statement to INSERT readings for sensor %s.  Error: %s",set.sensorName.c_str(),sqlite3_errmsg(db));
        }    
    } else {
//        syslog(LOG_DAEMON|LOG_ERR, "Fewer readings than expected (%d) received (%d) for SQLiteWriter.  Disregarding this reading.", fields.size(), set.readings.size());
    }
    pathMutex.unlock();
}

void LirSQLiteWriter::stopLogging(){
    if(this->logging){
        pathMutex.lock();
        if(db) sqlite3_close(db);
        db = NULL;
        pathMutex.unlock();
        this->logging = false;
        syslog(LOG_DAEMON|LOG_INFO, "Sqlite3 database @ %s closed.",dbPath.c_str());
    }
}

void LirSQLiteWriter::sensorStopping(){
}
