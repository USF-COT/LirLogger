
#include <string>
#include <iostream>
#include <syslog.h>

#include "LirCommand.hpp"
#include "LirTCPServer.hpp"


//Connection methods
tcp::socket& tcp_connection::socket(){
    return socket_;
}

tcp_connection::tcp_connection(boost::asio::io_service& io_service):socket_(io_service){}

void tcp_connection::start(){
    syslog(LOG_DAEMON|LOG_INFO,"New client connected.");
    boost::asio::async_read_until(socket_,buf,"\n",boost::bind(&tcp_connection::handle_line,shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void tcp_connection::handle_line(const boost::system::error_code& error, size_t bytes_transferred){
    LirCommand* cmd = LirCommand::Instance();

    istream is(&buf);
    string line;
    getline(is,line);

    string key = line.substr(0,line.find_first_of(" \n\r"));

    if(key.compare("listen") == 0){

    } else if(key.compare("silence") == 0){
        
    } else if(key.compare("exit") == 0){
        return;
    }
    string response = cmd->parseCommand(line);

    socket_.write_some(boost::asio::buffer(response));

    boost::asio::async_read_until(socket_,buf,"\n",boost::bind(&tcp_connection::handle_line,shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
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
