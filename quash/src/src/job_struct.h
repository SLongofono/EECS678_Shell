#ifndef JOB_STRUCT
#define JOB_STRUCT

#include "pid_queue.h"
/*
 * @brief The job_struct type is used to manage batches of running processes
 */
typedef struct{
	/* The arbitrary id associated with this job  */
	int job_id;
	
	/*  Stores the pids of the processes associated with this job, in
	 *  order that they were created */
	pid_queue process_q;
	
	/* Stores the current command buffer in a human-friendly format for
	 * this job */
	char* command;
} job_struct;

#endif
