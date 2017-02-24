/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 */

#include "execute.h"
#include <stdio.h>
#include "quash.h"



/****************************************************************************
 * Globals
 ***************************************************************************/

// Global job queue handle
// Used to track background job structs between calls
static job_queue bg_q;

// Flag such that above is only initialized once
static int first_time = true;

// Global pipe handles and tracking, used in create_process()
static int env_pipes[2][2];
static int in_pipe = -1;
static int out_pipe = 0;


/****************************************************************************
 * Destructors
 ***************************************************************************/

/*
 * @brief Frees memory for the background queue on exit
 *
 * signalled upstream from quash.c on exit condition
 *
 */
void free_background_queue(){
	
	assert(is_empty_job_queue(&bg_q));
	
	if(first_time != true){
		destroy_job_queue(&bg_q);
	}
}


/*
 * @brief Destructor for job_structs
 *
 * @note 	This is passed to the deque data structure in place of the
 * 		default destructor
 */
void destroy_struct(job_struct job){
	free(job.command);
	destroy_pid_queue(&job.process_q);
}



/***************************************************************************
 * Interface Functions
 ***************************************************************************/


// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {

	long max_path = fpathconf(0, _PC_PATH_MAX);

	// If no limit set, use a good rule of thumb value
	if(max_path < 0){
		max_path = 4096;
	}

	// Change this to true if necessary
	*should_free = true;

	char *temp = malloc(4096);
	getcwd(temp, max_path);

	return temp;
}


// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {

	return getenv(env_var);

}


// Check the status of background jobs
void check_jobs_bg_status() {
	int active;
	int status;
        job_struct temp_job_struct;// = new_job_queue(5);

	while(!is_empty_job_queue(&bg_q)){
		temp_job_struct = pop_front_job_queue(&bg_q);
		pid_queue *temp_q = &(temp_job_struct.process_q);
		while(!is_empty_pid_queue(temp_q)){
			active = pop_front_pid_queue(temp_q);
			// If any given process in the job returns a zero
			// value, then it is still running and we know the
			// job is still active.
			if(waitpid(active, &status, WNOHANG)==0) {
				push_front_pid_queue(temp_q, active);
				push_front_job_queue(&bg_q, temp_job_struct);
				return;
			}
		}
		// Print completion message
	 	print_job_bg_complete(temp_job_struct.job_id, active, temp_job_struct.command);
		
		// Clean up the job
		destroy_pid_queue(&temp_job_struct.process_q);
		free(temp_job_struct.command);
	}
#if 0

		printf("job_dead is %d, wrapping up check bg status...\n", job_dead);

		// Grab info and Clean up
		//destroy_pid_queue(temp_q);
		jid = temp_job_struct.job_id;
		
		printf("Job id: %d\n", jid);
	

 		// fails here
		pid = peek_front_pid_queue(&temp_job_struct.process_q);
		
		printf("Process id: %d\n", pid);

		// If the job is not done, put it into the temp queue to be written
		// back after all are checked
		if(!job_dead){
			printf("Job still alive, adding to temp queue...\n");
			push_back_job_queue(&temp_job_q, temp_job_struct);
		}
		else{
			printf("Job dead, printing completion msg..\n");
			// Otherwise, print out the completion message
			print_job_bg_complete(jid, pid, get_command_string());
		}
	}
	// Refill job queue and clean up
	
	printf("Filling bg queue again...\n");
	while(!is_empty_job_queue(&temp_job_q)){
		push_back_job_queue(&bg_q, pop_front_job_queue(&temp_job_q));	
	}

#endif
}


// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
	printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
	fflush(stdout);
}


// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
	printf("Background job started: ");
	print_job(job_id, pid, cmd);
}


// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
	printf("Completed: \t");
	print_job(job_id, pid, cmd);
}


/***************************************************************************
 * Functions to process commands
 ***************************************************************************/


// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {

	// Execute a program with a list of arguments. The `args` array is a NULL
	// terminated (last string is always NULL) list of strings. The first element
	// in the array is the executable
	char* exec = cmd.args[0];
	char** args = cmd.args;

	execvp(exec, args);

	perror("ERROR: Failed to execute program");
}


// Print strings
void run_echo(EchoCommand cmd) {
	// Print an array of strings. The args array is a NULL terminated (last
	// string is always NULL) list of strings.
	char** str = cmd.args;
	while(NULL != *str){
		printf("%s", *str);
		str++;
	}

	putchar('\n');

	// Flush the buffer before returning
	fflush(stdout);
}


// Sets an environment variable
void run_export(ExportCommand cmd) {

	// Simply try to set the environment varible to the given value, no
	// need to check that it is accurate
	setenv(cmd.env_var, cmd.val, 1);
	
}


// Changes the current working directory
void run_cd(CDCommand cmd) {

	// Get the directory name
	const char* dir = cmd.dir;

	// Check if the directory is valid
	if (dir == NULL) {
		perror("ERROR: Failed to resolve path");
		return;
	}

	const char *temp = getenv("PWD");

	// Actually change the working directory
	if(0 != chdir(dir) ){
		perror("ERROR: Failed to change directory");
		return;
	}

	// Change environment variables
	if( 0 != setenv("OLDPWD", temp, 1) ){
		perror("ERROR: Failed to update OLDPWD");
		return;
	}
	else if( 0 != setenv("PWD", dir, 1) ){
		perror("ERROR: Failed to update PWD");
		return;
	}
}


// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
	int signal = cmd.sig;
	int job_id = cmd.job;

	// TODO: Remove warning silencers
	(void) signal; // Silence unused variable warning
	(void) job_id; // Silence unused variable warning

	// TODO: Kill all processes associated with a background job
	//IMPLEMENT_ME();

	int num_jobs = length_job_queue(&bg_q);
	for(int i = 0; i<num_jobs; i++){
		// Grab the front
		job_struct temp = pop_front_job_queue(&bg_q);
		
		if(job_id == temp.job_id){
			size_t num_pids = length_pid_queue(&temp.process_q);
			
			for(int j = 0; j<num_pids; ++j){
				// Grab pid and send signal
				int pid = pop_front_pid_queue(&temp.process_q);
				kill(pid, signal);

				// Put it back in order
				push_back_pid_queue(&temp.process_q, pid);
			}

		}

		// Put job back in place
		push_back_job_queue(&bg_q, temp);
	}

}


// Prints the current working directory to stdout
void run_pwd() {

	// Per instructions, we can't just print the value of $PWD
	long max_path = fpathconf(0, _PC_PATH_MAX);
	
	if(0 > max_path){
		max_path =4096;	
	}

	char *temp = malloc(max_path);
	//char *temp;

	// Uses Malloc
	//temp = get_current_dir_name();
	
	getcwd(temp, max_path);

	printf("%s\n", temp);
	
	// Flush the buffer before returning
	fflush(stdout);
	
	free(temp);

}


// Prints all background jobs currently in the job list to stdout
// USING THE FORMAT [job_id]<tab>#PID#<tab>commandstring
void run_jobs() {

	int num_jobs = length_job_queue(&bg_q);
	
	for(int i = 0; i<num_jobs; i++){
		// Grab the front
		job_struct temp = pop_front_job_queue(&bg_q);

		printf("[%d]\t#PID#\t%s\n", temp.job_id, temp.command);

		// Put job back in place
		push_back_job_queue(&bg_q, temp);
	}

	fflush(stdout);

// hidden in the bowels of the documentation, there is a helpful message
// saying that we must use the above format
	
}


/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/


/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
	CommandType type = get_command_type(cmd);

	switch (type) {
	case GENERIC:
	  run_generic(cmd.generic);
	  break;

	case ECHO:
	  run_echo(cmd.echo);
	  break;

	case PWD:
	  run_pwd();
	  break;

	case JOBS:
	  run_jobs();
	  break;

	case EXPORT:
	case CD:
	case KILL:
	case EXIT:
	case EOC:
	  break;

	default:
	  fprintf(stderr, "Unknown command type: %d\n", type);
	}
}


/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {

	CommandType type = get_command_type(cmd);

	switch (type) {
		case EXPORT:
			run_export(cmd.export);
			break;

		case CD:
			run_cd(cmd.cd);
			break;

		case KILL:
			run_kill(cmd.kill);
			break;

		case GENERIC:
		case ECHO:
		case PWD:
		case JOBS:
		case EXIT:
		case EOC:
			break;

		default:
		fprintf(stderr, "Unknown command type: %d\n", type);
	}

}


/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, job_struct *job) {

	// Read the flags field from the parser
	bool p_in  = holder.flags & PIPE_IN;
	bool p_out = holder.flags & PIPE_OUT;
	bool r_in  = holder.flags & REDIRECT_IN;
	bool r_out = holder.flags & REDIRECT_OUT;
	bool r_app = holder.flags & REDIRECT_APPEND;
	// This can only be true if r_out is true

	// Create only one pipe each time.  Start with the out pipe.  Since we
	// can depend on the parser to never have p_out true on the final
	// process in a pipe, we know we will always have exactly n-1 pipes
	// for n processes.
	if(p_out){
		pipe(env_pipes[out_pipe]);	
	}	

	int pid = fork();
	if(0 == pid){  // Child process

		if(p_in || p_out){

			// If p_in, we need to connect the read end of the
			// global input pipe.  The values of the indices are
			// updated at the end of this function
			if(p_in){
				dup2(env_pipes[in_pipe][0], 0);
				close(env_pipes[in_pipe][1]);
			}
	
			// If p_out, we need to connect the write end of the
			// global output pipe.  The values of the indices are
			// updated at the end of this function
			if(p_out){
				dup2(env_pipes[out_pipe][1],1);
				close(env_pipes[out_pipe][0]);
			}
		}


		if(r_in){
		
			// Open the file at the given path in read only mode
			int fp = open(holder.redirect_in, O_RDONLY);
			if (fp < 0){
				perror("Error: could not open file for input redirection");
				return;
			}

			// Duplicate file descriptor to replace stdin
			dup2(fp, 0);

			// close now that we don't need it anymore
			close(fp);
		}
		if(r_out){
			
			if(r_app){

				// open the file at the given path in
				// read/write/create mode (append if exists)
				int fp = open(holder.redirect_out, O_RDWR | O_APPEND | O_CREAT, 0777);

				if(fp < 0){
					perror("ERROR: could not open file for output redirection");
					return;
				}

				// duplicate the file descriptor to replace
				// stdout
				dup2(fp, 1);

				// close now that we don't need it anymore
				close(fp);
			}
			else{
				
				// open the fail at the given path in
				// write/create mode (overwrite if exists)

				/* ***NOTE***
				 *
				 *  The mode needs to come at the end,
				 *  otherwise the options will be split up at
				 *  the comma and everything thereafter will
				 *  be ignored.  This was a stupid bug that
				 *  caused me a lot of aggravation, mostly
				 *  because there was no good way to search
				 *  for the error.
				 *
				 */
				int fp = open(holder.redirect_out, O_WRONLY | O_TRUNC | O_CREAT, 0777);

				if(fp < 0){
					perror("Error: could not open file for output redirection");
					return;	
				}

				// duplicate the file descriptor to replace
				// stdout
				dup2(fp, 1);

				// close now that we don't need it anymore
				close(fp);
			}
		} // end if(r_out)

	child_run_command(holder.cmd); // This should be done in the child branch of a fork

	exit(0);
	
	}// end if(0 == pid), child process block
	else{
		// Since we only create "out" pipes in parent, we need only
		// close the out pipe write end
		if(p_out){
			close(env_pipes[out_pipe][1]);	
		}

		// This is some hackery passed along by our TA.  It simply
		// alternates them between 0 and 1 for use in the global pipe
		// handles.  The assumption is that proc 1 will complete and
		// be done with pipe 0 in time for proc 3 to use it, as
		// enforced by the fact that each process will block until
		// data arrives from the previous process.
		//
		// Here is a sweet picture in case you forget later
		//
		//
		// Stage 1:
		//		__________		__________
		// proc 1	__pipe 0__	proc 2	__pipe 1__
		//
		// 
		//
		// Stage 2:
		//		__________		__________
		// proc 3 write	__pipe 0__	proc 2	__pipe 1__ proc 3 read
		//
		//
		// and so on...
		//
		//
		out_pipe = (out_pipe + 1) % 2;
		in_pipe = (in_pipe + 1) % 2;

		// Guess what I do
		parent_run_command(holder.cmd);
	}

	// Add the child to the active foreground process queue
	push_back_pid_queue(&(job->process_q), pid);

}


/**
 *
 * @brief Creates processes for foreground and background queues
 *
 * @param holders	A list of CommandHolder types populated by the parser
 * 			representing commands to be issued
 *
 * @note 	Create_process() does the heavy lifting with process creation,
 * 		redirection, and piping.  This script enforces the order of
 * 		execution, and separates the job into the background job queue
 * 		if necessary.
 */
void run_script(CommandHolder* holders) {

	// We only want to instantiate the bg_q once.  This is better encapsulated
	// than the previous implementation with the globals in quash.c
	if (first_time == true) {
		bg_q = new_destructable_job_queue (1, &destroy_struct);
		first_time = false;
	}

	if (holders == NULL)
	  return;

	// This enforces the order of operations.  Background jobs will only
	// report completion when the next command is entered.
	check_jobs_bg_status();

	if (get_command_holder_type(holders[0]) == EXIT &&
	    get_command_holder_type(holders[1]) == EOC) {
	  end_main_loop();
	  return;
	}

	job_struct the_job;

	// Global pid queue handle
	the_job.process_q = new_pid_queue(1);

	CommandType type;

	// Run all commands in the `holder` array
	for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i){
		create_process(holders[i], &the_job);
	}

	// Foreground jobs should be completed immediately
	if (!(holders[0].flags & BACKGROUND)) {
		
		// We need to wait for each to complete, then remove it from
		// our queue of foreground processes
		while(false == is_empty_pid_queue(&the_job.process_q)){

			int active, status;
			// Grab the first item (presumably the oldest if we
			// always add to the back)
			active = pop_front_pid_queue(&the_job.process_q);

			// Block until the associated process exits
			waitpid(active, &status, 0);
		}
		// Clean up process queue
		destroy_pid_queue(&the_job.process_q);
	}
	else {
	 	int jid, pid;
		// Make a struct to represent the job
		// get the last pid

		if(!is_empty_job_queue(&bg_q)){
			// If there are jobs queued, the next job id is one
			// more than that of the last job queued
			jid = peek_back_job_queue(&bg_q).job_id + 1;
		}
		else{
			jid = 1;	
		}

		the_job.job_id = jid;

		pid = peek_front_pid_queue(&the_job.process_q);

		// Set command string before we leave the active context
		// (since we don't know when it will complete and need to
		// print again)
		the_job.command = get_command_string();

		
		// Load job into background queue, print feedback, return
		// control to main block
		push_back_job_queue(&bg_q, the_job);
		print_job_bg_start(jid, pid, the_job.command);
	}
}// end run_script()

//END execute.c
