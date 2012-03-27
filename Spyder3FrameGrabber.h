/* Spyder3 Frame Grabber
 * - Wraps pleora eBUS SDK drivers into a smaller library that has just a few, Lir-Specific calls.
 *
 * By: Michael Lindemuth
 */

#include <PvSystem.h>
#include <PvInterface.h>
#include <PvDevice.h>
#include <PvStream.h>
#include <PvStreamRaw.h>
#include <PvBuffer.h>

#include <pthread.h>
#include <signal.h>

#include "FrameWriteBuffer.h"

struct Sp3GrabThreadParams{
    volatile sig_atomic_t running;
    volatile sig_atomic_t stopped;
    PvDevice device;
    PvStream stream;
    PvDeviceInfo *devInfo;
    pthread_mutex_t streamMutex;
    FrameWriteBuffer* writeBuffer;

    // Capture stats
    PvGenInteger *imageCount;
    PvGenFloat *frameRate;
    PvGenFloat *bandwidth;
};

class Spyder3FrameGrabber{
    private:
        Sp3GrabThreadParams runParams;
        pthread_t runner;

    public:
        Spyder3FrameGrabber();
        bool connect(char* MACAddress);
        bool startStream(FrameWriteBuffer* frameQueue);
        bool stopStream();
        void disconnect();
        ~Spyder3FrameGrabber();
        void ReQueueBuffer(PvBuffer* buffer);
};

