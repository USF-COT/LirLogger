/* Spyder3 Frame Grabber
 * - Wraps pleora eBUS SDK drivers into a smaller library that has just a few, Lir-Specific calls.
 *
 * By: Michael Lindemuth
 */

#include "Spyder3FrameGrabber.h"

/*
class Spyder3FrameGrabber{
    private:
        std::string configPath;
        unsigned int MAX_BUFFERS;
        PvDevice device;
        PvStream stream;

    public:
        Spyder3FrameGrabber(std::string path, std::queue<PvBuffer*> *frameQueue);
        Spyder3FrameGrabber(std::string path, std::queue<PvBuffer*> *frameQueue, unsigned int numBuffers);
        bool start();
        bool stop();
        ~Spyder3FrameGrabber();
        void ReQueueBuffer(PvBuffer* buffer);
};
*/

Spyder3FrameGrabber::Spyder3FrameGrabber(std::string path, std::queue<PvBuffer*> *frameQueue){

}

Spyder3FrameGrabber::Spyder3FrameGrabber(std::string path, std::queue<PvBuffer*> *frameQueue, unsigned int numBuffers){

}

bool Spyder3FrameGrabber::start(){

}

bool Spyder3FrameGrabber::stop(){

}

Spyder3FrameGrabber::~Spyder3FrameGrabber(){

}

void Spyder3FrameGrabber::ReQueueBuffer(PvBuffer* buffer){

}

