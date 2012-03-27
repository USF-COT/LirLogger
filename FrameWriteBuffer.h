/* FrameWriteBuffer - Defines a frame write buffer class to be used between producer and consumer threads
 *
 * By: Michael Lindemuth
 */

#ifndef FRAME_WRITE_BUFFER_H
#define FRAME_WRITE_BUFFER_H

#include <semaphore.h>
#include <pthread.h>
#include <PvBuffer.h>
#include <time.h>
#include <queue>

class FrameWriteBuffer{
    private:
        sem_t signal;
        pthread_mutex_t mutex;
        std::queue<PvBuffer*> buffer;
        unsigned int limit;

    public:
        FrameWriteBuffer();
        FrameWriteBuffer(unsigned int limit);
        ~FrameWriteBuffer();
        unsigned int getLimit();
        bool enqueue(PvBuffer* buf);
        PvBuffer* dequeue();
};

#endif
