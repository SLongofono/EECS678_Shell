/*
 * FILE		: job_queue.c
 *
 * BRIEF	: Generates functions for operating on job_queue_t data
 * 		  structure
 */

#include "job_queue.h"

/*
 *Use the prototype macro to place all the job_queue related functions here
 */
IMPLEMENT_DEQUE (job_queue, job_struct);
