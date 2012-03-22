/* FrameWriteBuffer - Defines a frame write buffer class to be used between producer and consumer threads
 *
 * By: Michael Lindemuth
 */

#include "FrameWriteBuffer.h"

/*
class FrameWriteBuffer{
    private:
        sem_t signal;
        pthread_mutex_t mutex;
        queue<PvBuffer*> buffer;
        unsigned int limit;

    public:
        FrameWriteBuffer();
        FrameWriteBuffer(unsigned int limit);
        ~FrameWritebuffer();
        bool enqueue(PvBuffer* buffer);
        PvBuffer* dequeue();
};
*/

FrameWriteBuffer::FrameWriteBuffer(){
    sem_init(&signal,0,0);
    pthread_mutex_init(&mutex,NULL); 
    limit = 32; //arbitrary
}

FrameWriteBuffer::FrameWriteBuffer(unsigned int limit){
    FrameWriteBuffer();
    limit = limit;
}

FrameWriteBuffer::~FrameWriteBuffer(){
    pthread_mutex_destroy(&mutex);
    sem_destroy(&signal);
}

bool FrameWriteBuffer::enqueue(PvBuffer* buf){
    bool retVal = false;

    pthread_mutex_lock(&mutex);
    if(buffer.size() <= limit){
        buffer.push(buf);
        sem_post(&signal);
        retVal = true;
    }
    pthread_mutex_unlock(&mutex);

    return retVal;
}

PvBuffer* FrameWriteBuffer::dequeue(){
    const int WAIT_TIME_SECS = 2;
    PvBuffer* retVal = NULL;
    int waitRetVal = 0;

    timespec spec;
    spec.tv_sec = time(NULL) + WAIT_TIME_SECS;

    waitRetVal = sem_timedwait(&signal,&spec);
    if(waitRetVal == 0){
        pthread_mutex_lock(&mutex);
        retVal = buffer.front();
        buffer.pop();
        pthread_mutex_unlock(&mutex);
    }
    return retVal;
}

