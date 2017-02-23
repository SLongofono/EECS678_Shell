/*
 * FILE		: job_queue.h
 *
 * BRIEF	: Generates declarations for deque data structure to store
 * 		  job structs
 */

#ifndef __JOB_QUEUE_H__
#define __JOB_QUEUE_H__

#include "job_struct.h"
#include "deque.h"

/*
 * Declare the queue data structure
 */
IMPLEMENT_DEQUE_STRUCT (job_queue, job_struct);

/*
 * Declare a queue for storing job structs
 */
PROTOTYPE_DEQUE (job_queue, job_struct);

#endif /* __JOB_QUEUE_H__ */
