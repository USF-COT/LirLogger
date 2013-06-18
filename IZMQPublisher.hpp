
#ifndef IZMQPUBLISHER_HPP
#define IZMQPUBLISHER_HPP

class IZMQPublisher{
    public:
        virtual ~IZMQPublisher(){}
        virtual unsigned int getStreamPort() = 0;
};

#endif
