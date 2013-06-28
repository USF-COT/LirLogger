
#include "ZMQSendUtils.hpp"

void sendLirMessage(zmq::socket_t& socket, char prefix, unsigned int ID, time_t timestamp, string data){
    zmq::message_t prefixMessage(1);
    memcpy((void*) prefixMessage.data(), &prefix, 1);
    socket.send(prefixMessage, ZMQ_SNDMORE);

    zmq::message_t keyMessage(sizeof(ID));
    memcpy((void*) keyMessage.data(), &ID, sizeof(ID));
    socket.send(keyMessage, ZMQ_SNDMORE);

    zmq::message_t timeMessage(sizeof(timestamp));
    memcpy((void*)timeMessage.data(), &timestamp, sizeof(timestamp));
    socket.send(timeMessage, ZMQ_SNDMORE);

    zmq::message_t dataMessage(data.size());
    memcpy((void*)dataMessage.data(), data.data(), data.size());
    socket.send(dataMessage, 0);
}
