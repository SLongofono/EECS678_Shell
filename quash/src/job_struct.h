#ifndef JOB_STRUCT
#define JOB_STRUCT

#include "pid_queue.h"

typedef struct{
	int job_id;
	pid_queue* process_q;
} job_struct;

#endif
