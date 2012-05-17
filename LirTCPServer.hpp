
#ifndef LIRTCPSERVER_HPP
#define LIRTCPSERVER_HPP

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

class tcp_connection
    : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    tcp::socket& socket();

    void start();
    
private:
    tcp_connection(boost::asio::io_service& io_service);

    void handle_line(const boost::system::error_code& error, size_t bytes_transferred);
    void setupStatsTimer(const string line);
    void writeStats();

    tcp::socket socket_;
    boost::asio::streambuf buf;

    boost::mutex statsTimerMutex;
    bool writingStats;
    unsigned int statsWriteIntervalSeconds;
    boost::asio::deadline_timer statsWriteTimer;
};

class LirTCPServer{
public:
    LirTCPServer(boost::asio::io_service& io_service);

private:
    void start_accept();
    
    void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error);

    tcp::acceptor acceptor_;
};


#endif
