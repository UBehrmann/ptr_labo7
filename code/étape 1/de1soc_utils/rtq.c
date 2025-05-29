#include "rtq.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int rtq_init(rtq *rtq, size_t heap_cap, size_t msg_cap, unsigned id)
{
	int ret;
	rtq->mem = malloc(EVL_HEAP_RAW_SIZE(heap_cap));
	if (!rtq->mem) goto mem_fail;

	rtq->msg_queue = malloc(msg_cap * sizeof(*rtq->msg_queue));
	if(rtq->msg_queue == NULL) goto msg_fail;
	rtq->msg_cap = msg_cap;
	rtq->msg_size = 0;
	rtq->msg_wr_idx = 0;
	rtq->msg_rd_idx = 0;
	
	if((ret = evl_init_heap(&rtq->heap, rtq->mem, EVL_HEAP_RAW_SIZE(heap_cap))) < 0) goto heap_fail;

	if(evl_new_mutex(&rtq->mut, "rtq_mut%u", id) < 0) goto mutex_fail;

	if(evl_new_event(&rtq->ev, "rtq_ev%u", id) < 0) goto event_fail;

	return 0;

event_fail:
mutex_fail:
	evl_destroy_heap(&rtq->heap);
heap_fail:
	free(rtq->msg_queue);
msg_fail:
	free(rtq->mem);
mem_fail:
	return -1;
}

int rtq_alloc(rtq *rtq, rt_msg *msg)
{
	if((msg->data = evl_alloc_block(&rtq->heap, msg->size)) == NULL) return -1;
	return 0;
}

int rtq_publish(rtq *rtq, rt_msg *msg)
{
	if(rtq->msg_size == rtq->msg_cap) return -1; //No more space in queue
	evl_lock_mutex(&rtq->mut);
	//Copy message to queue (cyclic buffer)
	rtq->msg_queue[rtq->msg_wr_idx] = *msg;
	rtq->msg_size++;
	rtq->msg_wr_idx = (rtq->msg_wr_idx + 1) % rtq->msg_cap;
	//Wake reader
	evl_signal_event(&rtq->ev);
	evl_unlock_mutex(&rtq->mut);
	return 0;
}

int rtq_retrieve(rtq *rtq, rt_msg *msg)
{
	evl_lock_mutex(&rtq->mut);
	if (rtq->msg_size == 0) //Wait for available message or queue destruction
	{
		evl_wait_event(&rtq->ev, &rtq->mut);
	}
	if (rtq->msg_size == 0) return -1; // Queue destroyed
	// Copy message from queue (cyclic buffer)
	*msg = rtq->msg_queue[rtq->msg_rd_idx];
	rtq->msg_size--;
	rtq->msg_rd_idx = (rtq->msg_rd_idx + 1) % rtq->msg_cap;

	evl_unlock_mutex(&rtq->mut);
	return 0;
}

int rtq_free(rtq *rtq, rt_msg *msg)
{
	if (evl_free_block(&rtq->heap, msg->data) < 0) return -1;
	msg->data = NULL;
	msg->size = 0;
	return 0;
}

void rtq_destroy(rtq *rtq)
{
	evl_lock_mutex(&rtq->mut);
	// Empty queue
	for (size_t i = 0; i < rtq->msg_size; i++)
	{
		evl_free_block(&rtq->heap, rtq->msg_queue[rtq->msg_rd_idx].data);
		rtq->msg_rd_idx = (rtq->msg_rd_idx + 1) % rtq->msg_cap;
	}
	rtq->msg_size = 0;
	
	//evl_broadcast_event(&rtq->ev); // Wake thread blocked on event
	evl_signal_event(&rtq->ev);

	evl_destroy_heap(&rtq->heap);
	free(rtq->msg_queue);
	free(rtq->mem);
	evl_unlock_mutex(&rtq->mut);
}
