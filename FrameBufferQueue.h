/*
 * FrameBufferQueue.h - A thread-safe, limited sized linked queue of frame buffers
 * 
 * By: Michael Lindemuth
 */

#include <pthread.h>
#include <PvBuffer.h>

typedef struct FQUEUECELL{
    PvBuffer *buffer;
}frameQueueCell;

class FrameBufferQueue{
    private:
        unsigned int BUF_MAX;

    public:
        FrameBufferQueue();
        FrameBufferQueue(unsigned int bufferMax);
        bool enqueue(PvBuffer* buffer);
        frameQueueCell* dequeue();
}
