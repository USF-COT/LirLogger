
#include <string>
#include <sstream>
#include <syslog.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "LirCommand.hpp"
#include "LirTCPServer.hpp"


//Connection methods
tcp::socket& tcp_connection::socket(){
    return socket_;
}

tcp_connection::tcp_connection(boost::asio::io_service& io_service):socket_(io_service),statsWriteTimer(io_service){
}

void tcp_connection::start(){
    syslog(LOG_DAEMON|LOG_INFO,"New client connected.");

    // Setup client line write handler
    boost::asio::async_read_until(socket_,buf,"\n",boost::bind(&tcp_connection::handle_line,shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void tcp_connection::handle_line(const boost::system::error_code& error, size_t bytes_transferred){
    LirCommand* cmd = LirCommand::Instance();

    istream is(&buf);
    string line;
    getline(is,line);

    string key = line.substr(0,line.find_first_of(" \n\r"));

    if(key.compare("listen") == 0){
        setupStatsTimer(line);
    } else if(key.compare("silence") == 0){
        statsTimerMutex.lock();
        writingStats = false;
        statsTimerMutex.unlock();
    } else if(key.compare("exit") == 0){
        return;
    } else {
        string response = cmd->parseCommand(line);
        socket_.write_some(boost::asio::buffer(response));
    }

    boost::asio::async_read_until(socket_,buf,"\n",boost::bind(&tcp_connection::handle_line,shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void tcp_connection::setupStatsTimer(const string line){
    vector<string> tokens;
    split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
    
    statsTimerMutex.lock();
    statsWriteIntervalSeconds = 1;
    if(tokens.size() > 1){
        string lastElement = tokens.back();
        try{
            statsWriteIntervalSeconds = boost::lexical_cast<unsigned int>(lastElement);
        } catch (boost::bad_lexical_cast const &){
            syslog(LOG_DAEMON|LOG_ERR, "Cannot parse last argument in listen command string (%s) as number.",lastElement.c_str());
        }
    }
    writingStats = 1;
    statsTimerMutex.unlock();
    
    // Get the stats listener running
    statsWriteTimer.expires_from_now(boost::posix_time::seconds(statsWriteIntervalSeconds));
    statsWriteTimer.async_wait(boost::bind(&tcp_connection::writeStats,shared_from_this()));
}

void tcp_connection::writeStats(){
    LirCommand *cmd = LirCommand::Instance();
    Spyder3Stats stats = cmd->getCameraStats();

    stringstream ss;
    // Create line for camera
    ss << "CAM:" << stats.imageCount << "," << stats.framesDropped << "," << stats.frameRate << "\r\n";

    // Create line for each sensor
    vector<EthSensorReadingSet> sensorSets = cmd->getSensorSets();
    for(unsigned int i=0; i < sensorSets.size(); ++i){
        ss << sensorSets[i].sensorName << ":" << sensorSets[i].time;
        for(unsigned int j=0; j < sensorSets[i].readings.size(); ++j){
            ss << "," << sensorSets[i].readings[j].text;
        }
        ss << "\r\n";
    }

    socket_.write_some(boost::asio::buffer(ss.str()));

    // Run again?
    bool isRunning;
    statsTimerMutex.lock();
    isRunning = writingStats;
    statsTimerMutex.unlock();
    if(isRunning){
        statsWriteTimer.expires_from_now(boost::posix_time::seconds(statsWriteIntervalSeconds));
        statsWriteTimer.async_wait(boost::bind(&tcp_connection::writeStats,shared_from_this()));
    }
}


// Server methods
LirTCPServer::LirTCPServer(boost::asio::io_service& io_service):acceptor_(io_service, tcp::endpoint(tcp::v4(), 8045)){
    start_accept();
}

void LirTCPServer::start_accept(){
    tcp_connection::pointer new_connection = 
        tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),boost::bind(&LirTCPServer::handle_accept, this, new_connection, boost::asio::placeholders::error));
}

void LirTCPServer::handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error){
    if(!error){
        new_connection->start();
    }
    start_accept();
}
