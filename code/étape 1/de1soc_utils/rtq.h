#ifndef RTQ_H_
#define RTQ_H_
#include <stdint.h>

#include <evl/mutex.h>
#include <evl/event.h>
#include <evl/heap.h>

struct rt_msg
{
	void *data;
	size_t size;
};
typedef struct rt_msg rt_msg;

/**
 * Real-Time capable queue for RT thread communication. Currently, only one writer and one reader is handled
 */
struct rtq
{
	void *mem;
	rt_msg *msg_queue;
	size_t msg_cap;
	size_t msg_size;
	size_t msg_wr_idx;
	size_t msg_rd_idx;
	struct evl_heap heap;
	struct evl_mutex mut;
	struct evl_event ev;
};
typedef struct rtq rtq;


/**
 * Initialises the queue. Dynamically allocates the requested amount of memory. Not RT
 * 
 * @param rtq The rtq to init
 * @param heap_cap Max capacity for the RT heap
 * @param msg_cap Max number of message that can be stored in the queue
 * @param id Unique identifier for the queue
 * 
 * @returns <0 if error, 0 otherwise
 */
int rtq_init(rtq *rtq, size_t heap_cap, size_t msg_cap, unsigned id);

/**
 * Allocates a buffer for the message
 * 
 * @param rtq The queue to reserve in
 * @param msg The message to allocate memory to. The amount of memory allocated corresponds to the message's size attribute
 * 
 * @returns 0 if memory reserved successfully, <0 otherwise
 * 
 * Must only be used by the writer
 */
int rtq_alloc(rtq *rtq, rt_msg *msg);

/**
 * Publishes the message for the reader and wake it up
 * 
 * @param rtq The queue to publish to
 * @param size The message to publish
 * 
 * @returns <0 if error, 0 otherwise
 */
int rtq_publish(rtq *rtq, rt_msg *msg);

/**
 * Retrieves a message from the queue. Blocks until a message is available or the queue is destroyed
 * 
 * @param rtq The queue to retrieve from
 * @param size The message to retrieve
 * 
 * @returns <0 if error or queue destroyed, 0 otherwise
 */
int rtq_retrieve(rtq *rtq, rt_msg *msg);

/**
 * Frees memory from a message so that it may be used by the writer again
 * 
 * @param rtq The queue to recover from
 * @param msg The message whose allocated space can be freed
 * 
 * @returns <0 if error, 0 otherwise
 * 
 * Must only be used by the reader
 */
int rtq_free(rtq *rtq, rt_msg *msg);

/**
 * Deallocates the rtq. Not RT
 * 
 * @param rtq The queue to destroy
 * 
 * Even once destroyed, the structure must not be altered until after the writer and reader threads are joined
 * WARNING: Make sure that no task is waiting on the queue as no mechanism is put in place to wake them during destruction
 */
void rtq_destroy(rtq *rtq);

#endif /* TEST_H_ */