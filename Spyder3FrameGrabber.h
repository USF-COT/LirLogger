/* Spyder3 Frame Grabber
 * - Wraps pleora eBUS SDK drivers into a smaller library that has just a few, Lir-Specific calls.
 *
 * By: Michael Lindemuth
 */

#include <PvConfigurationReader.h>
#include <PvDevice.h>
#include <PvStream.h>
#include <PvStreamRaw.h>
#include <PvBuffer.h>
#include <string>
#include <queue>
#include <pthread.h>

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

