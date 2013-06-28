
#ifndef ZMQSENDUTILS_HPP
#define ZMQSENDUTILS_HPP

#include <zmq.hpp>
#include <string>
#include <time.h>

using namespace std;

void sendLirMessage(zmq::socket_t& socket, char prefix, unsigned int key, time_t timestamp, string data);

#endif
